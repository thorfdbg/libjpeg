/*
** A couple of generic TMO related functions: Estimate TMO from LDR and HDR
** image pair, build a gamma mapping.
**
** $Id: tmo.hpp,v 1.7 2015/11/17 15:34:43 thor Exp $
**
*/

#ifndef CMD_TMO_HPP
#define CMD_TMO_HPP

/// Includes
#include "std/stdio.hpp"
#include "interface/types.hpp"
///

/// Prototypes
// Invert a tabulated tone mapping table. To be on the fair side,
// this uses the same algorithm the library uses to build the forwards
// table from the backwards table provided. Otherwise, I could also
// invert numerically the (parametric) table.
extern void InvertTable(UWORD input[65536],UWORD output[65536],UBYTE inbits,UBYTE outbits);
//
// Build an inverse tone mapping from a hdr/ldr image pair
extern void BuildToneMappingFromLDR(FILE *in,FILE *ldrin,int w,int h,int depth,int count,
				    UWORD ldrtohdr[65536],bool flt,bool bigendian,bool xyz,
				    int hiddenbits,bool median,bool &fullrange,
				    int smooth);
// Build an inverse tone mapping from a hdr/ldr image pair, though generate it as
// a floating point table. This requires floating point input.
extern void BuildToneMappingFromLDR(FILE *in,FILE *ldrin,int w,int h,int count,
				    FLOAT ldrtohdr[256],
				    bool bigendian,bool median,bool fullrange,
				    int smooth);
//
// Build three inverse TMOs from a hdr/ldr image pair.
extern void BuildRGBToneMappingFromLDR(FILE *in,FILE *ldrin,int w,int h,int depth,int count,
				       UWORD red[65536],UWORD green[65536],UWORD blue[65536],
				       bool flt,bool bigendian,bool xyz,
				       int hiddenbits,bool median,bool &fullrange,
				       int smooth);
//
// Build a static gamma mapping to map the HDR to the LDR domain.
extern void BuildGammaMapping(double gamma,double exposure,UWORD ldrtohdr[65536],
			      bool flt,int max,int hiddenbits);
//
// Load an inverse tone mapping table from a file.
extern void LoadLTable(const char *ltable,UWORD ldrtohdr[65536],bool flt,
		       int max,int hiddenbits);
///

///
#endif
