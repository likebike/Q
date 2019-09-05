#ifndef _SEB_DIRBASENAME_H
#define _SEB_DIRBASENAME_H

// The C standard library is really strange when it comes to dirname and basename:
//     * dirname is only defined in libgen.h.
//     * basename is defined in libgen.h and string.h.  The good GNU version is in string.h and the crappy XPG version is in libgen.h.
//     * If you include libgen.h, you can't use the GNU basename.
//     * ...So that means you can't use 'dirname' and the GNU 'basename' in the same code under normal circumstances.
//
// This module was created to enable the use of dirname and the GNU basename (without polluting your namespace with other GNU crap).

// Copy-pasted from string.h:
extern char *basename (__const char *__filename) __THROW __nonnull ((1));

// Copy-pasted from libgen.h:
extern char *dirname (char *__path) __THROW;

#endif
