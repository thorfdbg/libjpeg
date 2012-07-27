/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** $Id: unistd.hpp,v 1.2 2012-06-02 10:27:14 thor Exp $
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
