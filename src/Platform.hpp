//// Copyright (c) 2018 Silvio Mayolo
//// See LICENSE.txt for licensing details

#ifndef PLATFORM_HPP
#define PLATFORM_HPP

/// \file
///
/// \brief A bare bones system for detecting operating system
/// features.

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

/// This enumeration describes the different broad classes of
/// operating systems which Latitude recognizes.
enum class OS {

    /// Windows or WinNT operating systems.
    WINDOWS,

    /// Linux, Unix, or other POSIX-based operating systems.
    POSIX,

    /// Non-Windows and non-POSIX operating systems.
    UNKNOWN

};

#ifdef USE_WINDOWS
constexpr OS systemOS = OS::WINDOWS;
#endif // USE_WINDOWS

#ifdef USE_POSIX
constexpr OS systemOS = OS::POSIX;
#endif // USE_POSIX

#ifdef USE_NULL
constexpr OS systemOS = OS::UNKNOWN;
#endif // USE_NULL

#endif // PLATFORM_HPP
