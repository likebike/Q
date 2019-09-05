// LAST UPDATE: 2013-08-14

// Usage: int x = 57; HEX_DUMP(&x, sizeof(x));
#define HEX_DUMP(XX,SS) do{fprintf(stderr, "*"#XX" : "); \
                           seb_hexDump(stderr, XX, SS); \
                           fprintf(stderr, "\n");}while(0)

void (*seb_die)(char*, ...);

void seb_runTests();

void seb_hexDump(FILE *f, void *data, int size);

typedef struct {
    char *left;
    char *right;
} seb_strSplitPair_t;

seb_strSplitPair_t seb_strSplit_r1(char *s, char delim);

int seb_str_replace(char* src, char* dst, int dstLen, char* fromStr, int fromLen, char* toStr, int toLen);
char* seb_snprintf(char *buf, size_t bufLen, char *format, ...);
char* seb_itoa(char *buf, size_t bufLen, int i);
int seb_startswith(char* string, char* prefix);
int seb_endswith(char* string, char* suffix);


void seb_printStackTop(const char *type);





// Interesting Stuff:
// Lots of cool tips:  http://elinux.org/GCC_Tips

// /* definition to expand macro then apply to pragma message */
// #define VALUE_TO_STRING(x) #x
// #define VALUE(x) VALUE_TO_STRING(x)
// #define VAR_NAME_VALUE(var) #var "="  VALUE(var)
// 
// Examples:
// /* Some test definition here */
// #define DEFINED_BUT_NO_VALUE
// #define DEFINED_INT 3
// #define DEFINED_STR "ABC"
// 
// #pragma message(VAR_NAME_VALUE(NOT_DEFINED))
// #pragma message(VAR_NAME_VALUE(DEFINED_BUT_NO_VALUE))
// #pragma message(VAR_NAME_VALUE(DEFINED_INT))
// #pragma message(VAR_NAME_VALUE(DEFINED_STR))
// 
// /* Resulting output: */
// test.c:10:9: note: #pragma message: NOT_DEFINED=NOT_DEFINED
// test.c:11:9: note: #pragma message: DEFINED_BUT_NO_VALUE=
// test.c:12:9: note: #pragma message: DEFINED_INT=3
// test.c:13:9: note: #pragma message: DEFINED_STR="ABC"



// How to show the exact steps that GCC is going to perform:
// (The Compilation Plan)
// Use the -### arg.  For example:
// gcc  -### -Wall -std=c99 -o tst tst.c



// Display the built-in compiler/system definitions:
// gcc -dM -E - < /dev/null



// A good way to answer the question "Where is this symbol being defined?"
// 1) Add a "#error DIE" to the file that you want to dissect.
// 2) Run the build tool (make).  Capture the command that fails due to the #error.
// 3) Edit the command and add a "-dD -E" to the command.  This will produce a fully-
//    expanded source code, so you can see where everything is coming from.
//    Alternatively, you can use a "-C -E" to preserve comments.
// 4) View the resulting file and have your questions answered.




// When working with the GNU Standard C Library (and probably other libs too), you should "never" define __USE_GNU directly.  Instead, you should define _GNU_SOURCE.  Take a look at features.h for more info.  I guess that's what the double-underscore is meant to symbolize.



// If you do need to use a _GNU_SOURCE define, you should separate that piece into a separate C file so that the compilation and definition will be separated from the rest of the project.  This will prevent the define from poluting other sections of the code.


