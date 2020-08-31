/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/

/*
** This is an Os abstraction of the stdlib
** include file. It might possibly contain fixes for
** various Os derivations from the intended stdlib.
**
** $Id: unistd.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
*/

#ifndef UNISTD_HPP
#define UNISTD_HPP
#include "config.h"

#ifdef HAVE_BSTRINGS_H
#include <bstrings.h>
#elif defined(HAVE_BSTRING_H)
#include <bstring.h>
#endif

#ifdef HAVE_UNISTD_H

#if defined(HAVE_LLSEEK) || defined(HAVE_LSEEK64)
# ifndef _LARGEFILE64_SOURCE
#  define _LARGEFILE64_SOURCE
# endif
# include <sys/types.h>
#endif

#include <unistd.h>
// *ix compatibility hack, or rather, win compatibility hack
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif
#elif defined(HAVE_IO_H)
#include <io.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#endif

#if !defined(HAVE_OPEN) || !defined(HAVE_CLOSE) || !defined(HAVE_READ) || !defined(HAVE_WRITE)
#error "POSIX io functions are not available, won't compile without"
#endif

#ifndef HAVE_SLEEP
// Dummy implemented insite.
unsigned int sleep(unsigned int seconds);
#endif

// Provide definitions for STDIN_FILENO etc if not available.
#ifndef HAS_STDIN_FILENO
# define STDIN_FILENO 0
#endif

#ifndef HAS_STDOUT_FILENO
# define STDOUT_FILENO 1
#endif

#ifndef HAS_STDERR_FILENO
# define STDERR_FILENO 2
#endif

//
// A generic 64-bit version of seek
extern long long longseek(int fd,long long offset,int whence);

#endif
