// LAST UPDATE: 2013-09-04

#include <stdio.h>    // fprintf, fputs
#include <stdlib.h>   // malloc
#include <string.h>   // str functions
#include <stdarg.h>   // variadic args
#include <stdint.h>   // UINTPTR_MAX

#include "sebC.h"

#define EMERGENCY_BUF_SIZE 1024
char emergencyBuf[EMERGENCY_BUF_SIZE]; // Used during emergencies when we can't assume that malloc will work.

void seb_defaultErrHandler(char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    exit(99);
}
void (*seb_die)(char*, ...) = seb_defaultErrHandler;

void seb_hexDump(FILE *f, void *data, int size) {
    char *_data = (char *) data;
    int i;
    for(i=0; i<size; i++) {
        if(i!=0) fprintf(f, " ");
        fprintf(f, "%x", _data[i]);
    }
}

char* seb_snprintf(char *buf, size_t bufLen, char *format, ...) {
    va_list args;
    va_start(args, format);
    int charsWritten = vsnprintf(buf, bufLen, format, args);
    if(charsWritten==-1  ||  charsWritten>=bufLen) buf[0] = '\0';
    va_end(args);
    return buf;
}
char* seb_itoa(char *buf, size_t bufLen, int i) {
    return seb_snprintf(buf, bufLen, "%d", i);
}

seb_strSplitPair_t seb_strSplit_r1(char *s, char delim) {
    char *posP = strrchr(s, delim);
    int pos = posP ? posP - s : -1;
    int sLen = strlen(s);

    int leftLen = (pos==-1 ? 0 : pos);
    char *left = malloc(leftLen + 1);  // +1 for null.
    strncpy(left, s, leftLen+1);
    left[leftLen] = '\0';

    char *right = malloc(sLen - pos);
    strncpy(right, s+(pos+1), sLen-(pos+1));
    right[sLen] = '\0';

    seb_strSplitPair_t pair = {left, right};
    return pair;
}

int seb_startswith(char* string, char* prefix) {
    while(*prefix) {
        if(*prefix++ != *string++) return 0;
    }
    return 1;
}
int seb_endswith(char* string, char* suffix) {
    int Sl = strlen(string);
    int sl = strlen(suffix);
    if(sl > Sl) return 0;
    string = string+Sl-sl;
    while(*string) {
        if(*string++ != *suffix++) return 0;
    }
    return 1;
}



int seb_str_replace(char* src, char* dst, int dstLen, char* fromStr, int fromLen, char* toStr, int toLen) {
    // Searches 'src' and replaces all occurrences of 'fromStr' with 'toStr'.  Saves the null-terminated result in 'dst', using a maximum size of 'dstLen'.
    // If this function returns a non-zero value, the 'dst' must be considered invalid, and must NOT be used.
    int dstI = 0;
    char *fromPos;
    while( (fromPos = strstr(src, fromStr)) ) {
        int beforeLen = fromPos - src;
        if(beforeLen > dstLen-dstI-1) {
            fputs("seb_str_replace: Not enough dst for beforeStr!\n", stderr);
            return -10;
        }
        strncpy(&dst[dstI], src, beforeLen);
        dstI += beforeLen;

        if(toLen > dstLen-dstI-1) {
            fputs("seb_str_replace: Not enough dst for toStr!\n", stderr);
            return -11;
        }
        strncpy(&dst[dstI], toStr, toLen);
        dstI += toLen;
        src = &fromPos[fromLen];
    }
    // Add the ending:
    int afterLen = strlen(src);
    if(afterLen > dstLen-dstI-1) {
        fputs("seb_str_replace: Not enough dst for afterStr!\n", stderr);
        return -12;
    }
    strncpy(&dst[dstI], src, afterLen);
    dstI += afterLen;

    if(dstI > dstLen-1) {
        fputs("seb_str_replace: Not enough dst for null terminator!\n", stderr);
        return -13;
    }
    dst[dstI] = '\0';

    return 0;
}


















// Detect if we're working with a 32-bit or 64-bit architecture:
#if UINTPTR_MAX == 0xffffffff
#define SIZE_T_FORMAT "%u"  /* 32-bit */
#else
#define SIZE_T_FORMAT "%lu" /* Assume 64-bit */
#endif
size_t original_stack_address = 0;
void seb_printStackTop(const char *type) {
    // Useful for checking whether your tail-recursive funcion is operating properly.
    int stack_top;
    if(original_stack_address == 0) original_stack_address = (size_t) &stack_top;
    fprintf(stderr, "In %s call, the stack top offset is: " SIZE_T_FORMAT "\n", type, (original_stack_address - (size_t) &stack_top));
}




void seb_runTests() {
    int i1 = 0x12345678;
    HEX_DUMP(&i1, sizeof(i1));

    char s1[] = "abcdefgh";
    HEX_DUMP(s1, sizeof(s1));

    seb_strSplitPair_t p1;
    p1 = seb_strSplit_r1("abc:def", ':');
    HEX_DUMP(p1.left,4);
    HEX_DUMP(p1.right,4);

    p1 = seb_strSplit_r1("def", ':');
    HEX_DUMP(p1.left,4);
    HEX_DUMP(p1.right,4);

    p1 = seb_strSplit_r1(":def", ':');
    HEX_DUMP(p1.left,4);
    HEX_DUMP(p1.right,4);

    p1 = seb_strSplit_r1("abc:", ':');
    HEX_DUMP(p1.left,4);
    HEX_DUMP(p1.right,4);

    int bufSize = 10;
    char *buf = malloc(bufSize);
    strcpy(buf, "123456");
    HEX_DUMP(buf, bufSize+1);
    seb_itoa(buf, bufSize, 0);
    HEX_DUMP(buf, bufSize+1);
    seb_itoa(buf, bufSize, 10);
    HEX_DUMP(buf, bufSize+1);
    seb_itoa(buf, bufSize, 1000);
    HEX_DUMP(buf, bufSize+1);
    fprintf(stderr, "seb_itoa overflow result: '%s'\n", seb_itoa(buf, bufSize, 10000));
    HEX_DUMP(buf, bufSize+1);
    fprintf(stderr, "seb_itoa overflow result: '%s'\n", seb_itoa(buf, bufSize, -1000));
    HEX_DUMP(buf, bufSize+1);
}

