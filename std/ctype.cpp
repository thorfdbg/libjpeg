
#include "config.h"
#if !defined(HAVE_CTYPE_H) || !defined(HAVE_ISSPACE)
int isspace(int c)
{
  if (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == '\v')
    return 1;
  return 0;
}
#endif
