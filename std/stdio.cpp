
#include "stdio.hpp"

#ifndef HAVE_VSNPRINTF
int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  return vsprintf(str,format,ap);
}
#endif

#ifndef HAVE_SNPRINTF
int TYPE_CDECL snprintf(char *str,size_t size,const char *format,...)
{
    int result;
    va_list args;

    va_start(args,format);
    result = vsnprintf(str,size,format,args);

    va_end(args);

    return result;
}
#endif
