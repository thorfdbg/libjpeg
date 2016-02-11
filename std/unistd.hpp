
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
