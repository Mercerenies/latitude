#ifndef _PLATFORM_HPP_
#define _PLATFORM_HPP_

/*
 * This is where things start to get a bit messy. Ideally, we'd like to make this as similar
 * to the POSIX system as possible while maintaining support for reasonably modern Windows
 * operating systems as well.
 */

#if defined(__unix__)
#  define USE_POSIX
#elif defined(_WIN32)
#  define USE_WINDOWS
#else
#  define USE_NULL
#endif

#endif
