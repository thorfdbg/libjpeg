/*
** This file includes the decompressor front-end of the
** command line interface. It doesn't do much except
** calling libjpeg.
**
** $Id: reconstruct.hpp,v 1.1 2015/03/11 16:02:42 thor Exp $
**
*/

#ifndef CMD_RECONSTRUCT_HPP
#define CMD_RECONSTRUCT_HPP

/// Prototypes
extern void Reconstruct(const char *infile,const char *outfile,int colortrafo,const char *alpha);
///

///
#endif
