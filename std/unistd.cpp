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

/// Includes
#include "unistd.hpp"
#include "math.hpp"
///

#ifndef HAVE_SLEEP
#ifdef WIN32
// Comes with its own "sleep" version
#include <windows.h>
unsigned int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}
#else
unsigned int sleep(unsigned int seconds)
{
  int x;
  
  // Pretty stupid default implementation
  seconds <<= 8;
  while(seconds > 0) {
    double y = 1.1;
    for(x = 0;x < 32767;x++) {
      // Just keep it busy.
      y = pow(y,y);
      if (y > 2.0)
	y /= 2.0;
      
    }
    seconds--;
  }
  return 0;
}
#endif
#endif

  
/// longseek
// A generic 64-bit version of seek
long long longseek(int stream,long long offset,int whence)
{
#if defined(HAVE_LSEEK64)
  return lseek64(stream,offset,whence);
#elif defined(HAVE_LLSEEK)
  return llseek(stream,offset,whence);
#elif defined(HAVE_LSEEK)
  return lseek(stream,offset,whence);
#elif defined(HAVE__LSEEKI64)
  return _lseeki64(stream,offset,whence);
#else
  // No seeking possible.
  return -1;
#endif
}
///
