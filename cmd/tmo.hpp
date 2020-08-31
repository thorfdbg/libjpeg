/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

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
