#ifndef _SEB_NANOSLEEP_H
#define _SEB_NANOSLEEP_H

// All this stuff is copy-pasted from <time.h>.

struct timespec
  {
    __time_t tv_sec;		/* Seconds.  */
    long int tv_nsec;		/* Nanoseconds.  */
  };

extern int nanosleep (__const struct timespec *__requested_time,
		      struct timespec *__remaining);

#endif

