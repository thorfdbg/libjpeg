
/*
** An abstraction layer around the standard assert call. This also
** translates the jpeg CHECK_LEVEL into the apropriate macros for assert().
**
** $Id: assert.hpp,v 1.8 2014/09/30 08:33:17 thor Exp $
*/

#ifndef STD_ASSERT_HPP
#define STD_ASSERT_HPP

# if HAVE_ASSERT_H
#  if CHECK_LEVEL > 0
#   undef NDEBUG
#   include <assert.h>
#  else
#   define assert(x)
#  endif
# else
#  if CHECK_LEVEL > 0
#   define assert(x)  { if(!(x)) {volatile char *x = NULL; *x = 0;}}
#  else
#   define assert(x)
#  endif
# endif
#endif
