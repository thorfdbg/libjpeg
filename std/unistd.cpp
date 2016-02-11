
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
