// Q - By Christopher Sebastian and Ryan Sanden.  MIT License.
//        csebastian3@gmail.com     ryan@ryansanden.com
//
// This is a process queue, able to run a number of workers concurrently.
// Created to limit the number of concurrent CGI worker processes, but can
// probably be used for many other things too.
// Runs workers in a FIFO order (in the order they are launched).
//
// Limits:
//   1) Currently uses Record Locking for IPC and exec to launch the target process.
//      Therefore, if the target command does something like closing all file
//      descriptors, the lock will be released and this might mess up the
//      concurrency tracking (enabling many more to run at once).
//      I will solve this problem if/when I encounter it in real life.
//
//   2) You should never run this program as SUID.  It is not secure because it
//      is currently influenced by environment variables (HOME, PATH, etc), and
//      it currently uses the very powerful 'wordexp' function.
//      As long as this is not run with SUID, it should not expose any new
//      security vulnerabilities.


// TODO: Just use basename for arg[0] when exec'ing?  // Need more research.

char* VERSION = "Q Version 1.1.1";

#include <stdio.h>        // fputs, stderr
#include <unistd.h>       // execvp, sleep, getpid
#include <wordexp.h>      // wordexp
#include <pwd.h>          // getpwuid
#include <string.h>       // strlen, memset
#include <strings.h>      // strcasecmp
#include <stdlib.h>       // malloc, exit, atoi
#include <fcntl.h>        // open, fcntl
#include <errno.h>        // errno
#include <ctype.h>        // isdigit
#include <time.h>         // struct timespec
#include "abspath.h"      // abspath
#include "dir_base_name.h"// dirname, basename
#include "makedirs.h"     // makedirs
#include "nanosleep.h"    // nanosleep
#include "strdup.h"       // strdupa
#include "sebC.h"         // seb_snprintf

#define HEADER_SIZE 1  /* MasterLock */

// The indirection is nicely explained here: http://www.guyrutenberg.com/2008/12/20/expanding-macros-into-string-constants-in-c/
#define VALUE_TO_STRING(x) #x
#define VALUE(x) VALUE_TO_STRING(x)

int verbose = 0;

char* global_q_file_path = "???";
#define LOG_MSG_BUF_SIZE 4096
char logMsgBuf[LOG_MSG_BUF_SIZE];
char* L(char* msg) {  // L for "Log Message"
    return seb_snprintf(logMsgBuf, LOG_MSG_BUF_SIZE, "Q (%s): %s", global_q_file_path, msg);
}

char** duplicate_argv_with_null_termination(int argc, char** argv) {
    // The process's argv that we receive in main() is not able to be used by execvp for several reasons:
    //     1) We can't assume that we can modify it (for the first and last elements).
    //     2) We can't assume that those memory regions will behave normally during an exec.
    //     3) We can't assume that it is null-terminated.
    // Therefore, we need to duplicate the array ourselves before adding a null-terminator.
    char **argv0;
    argv0 = malloc((argc+1) * sizeof(*argv0)); if(!argv0) { fputs(L("out of memory\n"), stderr); exit(111); }
    for(int i=1; i<argc; i++) argv0[i] = argv[i];
    argv0[argc] = NULL;  // Must terminate the execvp args array with NULL.
    return argv0;
}

void run_target_cmd(char* cmd, char** argv0) {
    // argv0 must be null-terminated.
    argv0[0] = cmd;
    execvp(argv0[0], argv0);  // Note that execvp IS affected by PATH
    perror(L("exec"));
    exit(113);
}

int isIntString(char* s) {
    int l = strlen(s);
    if(!l) return 0;
    int i=0;
    if(s[i]=='+'  ||  s[i]=='-') {  // A leading sign is OK.
        i++;
        if(i >= l) return 0;  // No digits after sign.
    }
    for(; i<l; i++) {
        if(!isdigit(s[i])) return 0;
    }
    return 1;
}

char* expand_q_file_path(char* path) {
    // We use the very powerful wordexp() function, which could be dangerous if
    // used incorrectly.  For this reason, you should not SUID this program.
    char *finalPath = NULL;
    wordexp_t expansion = {0};
    int err = wordexp(path, &expansion, WRDE_NOCMD|WRDE_SHOWERR|WRDE_UNDEF);
    if(err) {
        switch(err) {
            case WRDE_NOSPACE:
                fputs(L("WRDE_NOSPACE (Ran out of memory.)\n"), stderr);
                break;
            case WRDE_BADCHAR:
                fputs(L("WRDE_BADCHAR (A metachar appears in the wrong place.)\n"), stderr);
                break;
            case WRDE_BADVAL:
                fputs(L("WRDE_BADVAL (Undefined var reference with WRDE_UNDEF.)\n"), stderr);
                break;
            case WRDE_CMDSUB:
                fputs(L("WRDE_CMDSUB (Command substitution with WRDE_NOCMD.)\n"), stderr);
                break;
            case WRDE_SYNTAX:
                fputs(L("WRDE_SYNTAX (Shell syntax error.)\n"), stderr);
                break;
            default: fputs(L("Unknown wordexp error!\n"), stderr);
        }
        goto onError;
    }
    if(expansion.we_wordc != 1) {
        fputs(L("Invalid q_file expansion.  Expected ONE word.\n"), stderr);
        goto onError;
    }
    finalPath = abspath(expansion.we_wordv[0], NULL);
    if(!finalPath) {
        perror(L("abspath"));
        fputs(L("Error converting expanded path to abspath:\n"), stderr);
        fprintf(stderr, "wordexp token: %s\n", expansion.we_wordv[0]);
        goto onError;
    }
    goto cleanup;
    
onError:
    if(finalPath) {
        free(finalPath);
        finalPath = NULL;
    }
cleanup:
    if(expansion.we_wordv) wordfree(&expansion);
    return finalPath;
}

void obtain_exclusive_lock(int fd) {
    while(1) {
        struct flock lock = {0};
        lock.l_type = F_WRLCK;
        lock.l_start = 0;
        lock.l_whence = SEEK_SET;
        lock.l_len = HEADER_SIZE;
        int retcode = fcntl(fd, F_SETLKW, &lock);
        if(!retcode) break;
        perror(L("fcntl(A)"));
        fputs(L("Trying again...\n"), stderr);
        sleep(1);  // Once I actually experience this in real life, I might decide to sleep longer or shorter.
    }
}
void release_exclusive_lock(int fd) {
    struct flock lock = {0};
    lock.l_type = F_UNLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 1;
    int retcode = fcntl(fd, F_SETLK, &lock);
    if(retcode) perror(L("fcntl(B)"));
}



int binary_search(int lowI, int highI, int (*targetIsLessThanMidpoint)(int, void*), void* data) {
    if(lowI == highI) return lowI;
    int midPoint = lowI + (highI-lowI)/2;
    if(midPoint == lowI) midPoint++;  // Due to the fact that we are using "less than" comparisons, take the higher one.
    int retcode = targetIsLessThanMidpoint(midPoint, data);
    if(retcode < 0) return retcode;  // Error.
    if(retcode) return binary_search(lowI, midPoint-1, targetIsLessThanMidpoint, data);  // Use Tail Recursion.
    return binary_search(midPoint, highI, targetIsLessThanMidpoint, data);               //
}

int isAnythingLockedAtOrAbove(int i, void* data) {  // data is 'fd'.
    if(i < 0) return 0; // Avoid going into the header.
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_start = HEADER_SIZE + i;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;  // 0 means 'everything'.
    int retcode = fcntl(*(int*)data, F_GETLK, &lock);
    if(retcode < 0) {
        perror(L("fcntl(C)"));
        return retcode;
    }
    return !lock.l_pid;
}
int isAnythingLockedBelow(int i, void* data) {
    if(i <= 0) return 0; // Handle separately because '0' len means 'everything'.
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_start = HEADER_SIZE;
    lock.l_whence = SEEK_SET;
    lock.l_len = i;
    int retcode = fcntl(*(int*)data, F_GETLK, &lock);    
    if(retcode < 0) {
        perror(L("fcntl(E)"));
        return retcode;
    }
    return !!lock.l_pid; 
}


#define SEARCH_MULTIPLIER 8
typedef struct _intPair {
    int first;
    int last;
} intPair;
intPair find_firstAndLast_slots(int fd, int forceLast) {
    intPair result = {-1, -1};
    // Simplify the following logic by doing this test first:
    // (We need to make this simplification because a binary search can't reach -1 in the case of no locks.)
    if(isAnythingLockedAtOrAbove(0, &fd)) return result; // There are no locks (other than ours).

    int retcode;
    if(forceLast >= 0) result.last = forceLast;
    else {
        // First, increase 'i' until we are past all locks:
        int i = 1;
        while( (retcode = isAnythingLockedAtOrAbove(i, &fd)) <= 0 ) {
            if(retcode < 0) return result; // Error.
            i *= SEARCH_MULTIPLIER;
        }

        // now we know that the end is somewhere between (i/SEARCH_MULTIPLIER) and (i), and we can use a binary search:
        retcode = binary_search(i/SEARCH_MULTIPLIER, i, isAnythingLockedAtOrAbove, &fd);
        if(retcode < 0) return result;  // Error.
        result.last = retcode;
    }

    // now we need to find the front, which is between 0 and result.last. We also use a binary search for this:
    retcode = binary_search(0, result.last, isAnythingLockedBelow, &fd);
    if(retcode < 0) return result;  // Error.
    result.first = retcode;

    return result;
}
// // The below function is useful for debugging.  Just un-comment and then
// // change the name of the above function to '_find_firstAndLast_slots':
// intPair find_firstAndLast_slots(int fd, int forceLast) {
//     intPair result = _find_firstAndLast_slots(fd, forceLast);
//     fprintf(stderr, "first=%d  last=%d\n", result.first, result.last);
//     return result;
// }

void wait_for_my_turn(char* q_file_path, int concurrency, int maxQueueSize, int autoCreateDirs) {
    // Here is a diagram of the file layout:
    //
    // MasterLock(byte) | Slot[0](byte) | Slot[1](byte) | ... | Slot[N](byte)
    //
    // The MasterLock is used to synchronize changes between processes.
    // The Slots are used for record locking -- one per process.

    if(autoCreateDirs) {
        char *parentDir = dirname(strdupa(q_file_path)); // Do NOT free() return value of dirname.
        makedirs(parentDir);
    }

    int fd = open(q_file_path, O_RDWR|O_CREAT, 0600);
    if(fd < 0) {
        perror(L("open"));
        return;
    }

    // Lock the file and claim a slot.
    obtain_exclusive_lock(fd);

    // At this point, we have exclusive control over the file.
    // We can do things non-atomically.
    intPair boundSlots = find_firstAndLast_slots(fd, -1);
    int mySlotNum = boundSlots.last + 1;
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_start = HEADER_SIZE + mySlotNum;
    lock.l_whence = SEEK_SET;
    lock.l_len = 1;
    int retcode = fcntl(fd, F_SETLK, &lock);
    if(retcode < 0) perror(L("fcntl(F)"));

    // Done with exclusive lock.
    release_exclusive_lock(fd);

    // Now wait for our turn:
#define MIN_SLEEP_TIME 0.05
#define MAX_SLEEP_TIME 10
    double secondsToSleep = MIN_SLEEP_TIME; // Use an exponential backoff.
    int prevActiveLocks = -1;
    for(;; boundSlots = find_firstAndLast_slots(fd, mySlotNum)) {
        // Before using slow iteration, just try to use logic and subtraction:
        if(boundSlots.first < 0) break; // We are the only thing in line.
        if(mySlotNum-boundSlots.first < concurrency) break;

        // We need to iterate:
        int i = boundSlots.first;
        int activeLocks = 1; // For My Lock.
        if(i < mySlotNum) {
            i++;  // Because we already know that boundSlots.first is locked.
            activeLocks++;  // Count the First as well as My Lock.
        }
        // In the loop test below, we do not want to test our own lock because we will be able to obtain it despite the lock.
        while(i<mySlotNum) {  // This used to be a 'for' loop, but GCC kept complaining that it had "no effect" so I switched it to a while loop to silence the warning.
            memset(&lock, 0, sizeof(lock));
            lock.l_type = F_WRLCK;
            lock.l_start = HEADER_SIZE + i;
            lock.l_whence = SEEK_SET;
            lock.l_len = 1;
            retcode = fcntl(fd, F_GETLK, &lock);
            if(retcode < 0) {
                perror(L("fcntl(G)"));
                break;
            }
            if(lock.l_pid) activeLocks++;
            i++;
        }
        if(activeLocks <= concurrency) break;
        if(maxQueueSize>=0  &&  activeLocks-concurrency>maxQueueSize) {
            fputs(L("MAX_Q exceeded!  Aborting.\n"), stderr);
            exit(99);
        }
        if(activeLocks != prevActiveLocks) {
            secondsToSleep = MIN_SLEEP_TIME * activeLocks / concurrency / 2;
            if(secondsToSleep < MIN_SLEEP_TIME) secondsToSleep = MIN_SLEEP_TIME;

            // OLD:
            //secondsToSleep *= 0.618;
            //double logicalMinTime = MIN_SLEEP_TIME * activeLocks / concurrency / 2;
            //if(secondsToSleep < logicalMinTime) secondsToSleep = logicalMinTime;
            //if(secondsToSleep < MIN_SLEEP_TIME) secondsToSleep = MIN_SLEEP_TIME;
        } else {
            secondsToSleep *= 1.1;
            if(secondsToSleep > MAX_SLEEP_TIME) secondsToSleep = MAX_SLEEP_TIME;

            // OLD:
            //secondsToSleep *= 1.382;
            //if(secondsToSleep > MAX_SLEEP_TIME) secondsToSleep = MAX_SLEEP_TIME;
        }
        prevActiveLocks = activeLocks;

        if(verbose) fprintf(stderr, "Q (%d): Waiting...\n", getpid());

        struct timespec secondsToSleep_struct;
        secondsToSleep_struct.tv_sec = (int)secondsToSleep;
        secondsToSleep_struct.tv_nsec = (long)((secondsToSleep - (int)secondsToSleep) * 1000000000);
        nanosleep(&secondsToSleep_struct, NULL);
    }
}

void help(char** argv) {
    fprintf(stderr, "\n");
    fprintf(stderr, "%s  --  a FIFO worker queue, capable of running multiple workers at once.\n", VERSION);
    fprintf(stderr, "\n");
    fprintf(stderr, "Typical usage:  COMMAND=/usr/bin/rsync  %s  [args...]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "The args get passed directly to COMMAND.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Environment Variables:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    COMMAND=path -- Required.  The path to the target executable.  Should usually be an absolute path.\n");
    fprintf(stderr, "    Q_FILE=path -- The path to the Q queue state file.  More info below.\n");
    fprintf(stderr, "    CONCURRENCY=num -- Controls the number of workers to run at once.  Default is 1.\n");
    fprintf(stderr, "    MAX_Q=num -- Controls the maximum queue size (# waiting to run).  Default is -1 (unlimited).\n");
    fprintf(stderr, "    AUTO_CREATE_DIRS -- Set to 0 to prevent the Q_FILE parent directories from being auto-created.\n");
    fprintf(stderr, "    VERBOSE -- Set to 1 to display verbose information.\n");
    fprintf(stderr, "    HELP -- Set to 1 to display this help message and then exit.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "By default, Q_FILE is set to ~/.tmp/Q/cmd_$(basename $COMMAND).Q, but you can change this.\n");
    fprintf(stderr, "You can use the tilde (~) and environment variables ($VAR) in the Q_FILE.\n");
    fprintf(stderr, "This enables you to achieve some interesting queuing structures:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    Q_FILE=example.Q                     # A relative path can queue jobs based on $PWD.\n");
    fprintf(stderr, "    Q_FILE=/tmp/shared.Q                 # A system-wide queue.  (You would need to chmod 666 it.)\n");
    fprintf(stderr, "    Q_FILE='~/myjobs.Q'                  # Use a per-account queue.\n");
    fprintf(stderr, "    Q_FILE='~/.tmp/Q/cgi_$SERVER_NAME.Q' # Assuming this is for CGI processes, use a per-domain queue.\n");
    fprintf(stderr, "\n");
#if defined(COMMAND) || defined(Q_FILE) || defined(AUTO_CREATE_DIRS) || defined(CONCURRENCY) || defined(MAX_Q)
    fprintf(stderr, "Hard-Coded Values (Defined at compile time):\n");
    fprintf(stderr, "\n");
#ifdef COMMAND
    fprintf(stderr, "    COMMAND=" VALUE(COMMAND) "\n");
#endif
#ifdef Q_FILE
    fprintf(stderr, "    Q_FILE=" VALUE(Q_FILE) "\n");
#endif
#ifdef CONCURRENCY
    fprintf(stderr, "    CONCURRENCY=" VALUE(CONCURRENCY) "\n");
#endif
#ifdef MAX_Q
    fprintf(stderr, "    MAX_Q=" VALUE(MAX_Q) "\n");
#endif
#ifdef AUTO_CREATE_DIRS
    fprintf(stderr, "    AUTO_CREATE_DIRS=" VALUE(AUTO_CREATE_DIRS) "\n");
#endif
    fprintf(stderr, "\n");
#endif
}

int readBooleanEnvVar(char* name, int defaultVal) {
    if(getenv(name) != NULL) {
        if(     !strcmp(getenv(name), "0")  ||
            !strcasecmp(getenv(name), "no")  ||
            !strcasecmp(getenv(name), "false") )
            return 0;
        else if(!strcmp(getenv(name), "1")  ||
            !strcasecmp(getenv(name), "yes")  ||
            !strcasecmp(getenv(name), "true") )
            return 1;
    }
    return defaultVal;
}

int main(int argc, char** argv) {

    int justHelp = readBooleanEnvVar("HELP", 0);
    if(justHelp) {
        help(argv);
        exit(0);
    }

#ifdef COMMAND
    char* command = VALUE(COMMAND);
#else
    char* command = getenv("COMMAND");
#endif
    if(!command) {
        help(argv);
        exit(1);
    }
    // Do some basic validation so we know it is OK to perform a 'basename' of the command.  It's not too important to perform a thorough check of the command -- we trust that the user is intelligent.  We just want to guard against common mistakes.
    if(!strcmp(command, "")         ||
       !strcmp(command, ".")        ||
       !strcmp(command, "..")       ||
       seb_endswith(command, "/")   ||
       seb_endswith(command, "/.")  ||
       seb_endswith(command, "/..") ) {
        fputs(L("Invalid COMMAND.  Aborting.\n"), stderr);
        exit(2);
    }
    

#ifdef Q_FILE
    char* q_file = VALUE(Q_FILE);
#else
    char* q_file = getenv("Q_FILE");
#endif
    if(q_file == NULL) {
        char *cmd_basename = basename(strdupa(command));
        if(!cmd_basename || strcmp(cmd_basename, "")==0) {
            fprintf(stderr, L("Unable to get basename of command (%s).  You should probably define the Q_FILE environment variable.  Aborting.\n"), command);
            exit(3);
        }
        char *valid_cmd_chars = "abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "0123456789"      "-_."
                                ",+:" /* some weird ones */ ;
        for(int i=0, bL=strlen(cmd_basename); i<bL; i++) {
            if(!strchr(valid_cmd_chars, cmd_basename[i])) {
                fputs(L("Unable to create default Q_FILE from COMMAND because of invalid characters.  You should probably define the Q_FILE environment variable.  Aborting.\n"), stderr);
                exit(4);
            }
        } 
#define DEFAULT_QFILE_LEN 1024
        q_file = malloc(DEFAULT_QFILE_LEN);
        if(!q_file) {
            perror(L("malloc"));
            exit(5);
        }
        seb_snprintf(q_file, DEFAULT_QFILE_LEN, "~/.tmp/Q/cmd_%s.Q", cmd_basename);
        if(!strcmp(q_file, "")) {
            fputs(L("Default Q_FILE path was too long!  You should probably define the Q_FILE environment variable.  Aborting.\n"), stderr);
            exit(5);
        }
    }
    if(!q_file) {
        fputs(L("q_file is NULL!  (This should never happen.)\n"), stderr);
        exit(5);
    }

    global_q_file_path = q_file;      // Work with what we've got.
    char* abs_q_file = expand_q_file_path(q_file);
    if(!abs_q_file) {
        fputs(L("Invalid Q_FILE path.  Skipping to launch...\n"), stderr);
        goto launchTime;
    }
    global_q_file_path = abs_q_file;  // Now switch to the nicer value.

#ifdef CONCURRENCY
    int concurrency = CONCURRENCY;
#else
    int concurrency = 1;
    if(getenv("CONCURRENCY") != NULL) {
        if(!isIntString(getenv("CONCURRENCY"))) {
            fputs(L("CONCURRENCY is non-integer!\n"), stderr);
            help(argv);
            exit(6);
        }
        concurrency = atoi(getenv("CONCURRENCY"));
    }
#endif
    if(concurrency < 1) {
        fputs(L("CONCURRENCY is less than 1!\n"), stderr);
        help(argv);
        exit(7);
    }

#ifdef MAX_Q
    int maxQueueSize = MAX_Q;
#else
    int maxQueueSize = -1;
    if(getenv("MAX_Q") != NULL) {
        if(!isIntString(getenv("MAX_Q"))) {
            fputs(L("MAX_Q is non-integer!\n"), stderr);
            help(argv);
            exit(8);
        }
        maxQueueSize = atoi(getenv("MAX_Q"));
    }
#endif
    if(maxQueueSize < -1) {
        fputs(L("MAX_Q is invalid!\n"), stderr);
        help(argv);
        exit(9);
    }

#ifdef AUTO_CREATE_DIRS
    int autoCreateDirs = AUTO_CREATE_DIRS;
#else
    int autoCreateDirs = readBooleanEnvVar("AUTO_CREATE_DIRS", 1);
#endif

    verbose = readBooleanEnvVar("VERBOSE", 0);  // Set the global variable.
    if(verbose) fprintf(stderr, "Q (%d): COMMAND=%s  Q_FILE=%s  CONCURRENCY=%d  MAX_Q=%d  AUTO_CREATE_DIRS=%d\n", getpid(), command, q_file, concurrency, maxQueueSize, autoCreateDirs);

    wait_for_my_turn(abs_q_file, concurrency, maxQueueSize, autoCreateDirs);

launchTime:
    if(verbose) fprintf(stderr, "Q (%d): Starting Command (%s)...\n", getpid(), command);
    run_target_cmd(command, duplicate_argv_with_null_termination(argc, argv));
    return 125; // We should never get here.
}
