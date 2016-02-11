/*
** This header provides the main function.
** This main function is only a demo, it is not part of the libjpeg code.
** It is here to serve as an entry point for the command line image
** compressor.
**
** $Id: main.hpp,v 1.9 2015/03/11 16:02:42 thor Exp $
**
*/

#ifndef CMD_MAIN_HPP
#define CMD_MAIN_HPP

/// Includes
#include "interface/types.hpp"
///

/// Prototypes
extern int main(int argc,char **argv);
extern void ParseSubsamplingFactors(UBYTE *sx,UBYTE *sy,const char *sub,int cnt);
///

///
#endif
