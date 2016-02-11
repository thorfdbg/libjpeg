/*************************************************************************

    This project implements a complete(!) JPEG (10918-1 ITU.T-81) codec,
    plus a library that can be used to encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Copyright (C) 2012-2015 Thomas Richter, University of Stuttgart and
    Accusoft.

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
** Parameter definition and encoding for profile C.
**
** $Id: encodec.hpp,v 1.14 2016/01/22 12:15:13 thor Exp $
**
*/

#ifndef CMD_ENCODEC_HPP
#define CMD_ENCODEC_HPP

/// Includes
#include "interface/types.hpp"
/// 

/// Prototypes
extern void EncodeC(const char *source,const char *ldrsource,
                    const char *target,const char *ltable,
                    int quality,int hdrquality,
                    int tabletype,int residualtt,int maxerror,
                    int colortrafo,bool lossless,bool progressive,
                    bool residual,bool optimize,bool accoding,
                    bool rsequential,bool rprogressive,bool raccoding,
                    bool dconly,UBYTE levels,bool pyramidal,bool writednl,UWORD restart,
                    double gamma,
                    int lsmode,bool noiseshaping,bool serms,bool losslessdct,bool dctbypass,
                    bool openloop,bool deadzone,bool xyz,bool cxyz,
                    int hiddenbits,int riddenbits,int resprec,bool separate,
                    bool median,int smooth,bool noclamp,
                    const char *sub,const char *ressub,
                    const char *alpha,int alphamode,int matte_r,int matte_g,int matte_b,
                    bool alpharesiduals,int alphaquality,int alphahdrquality,
                    int alphatt,int residualalphatt,
                    int ahiddenbits,int ariddenbits,int aresprec,
                    bool aopenloop,bool adeadzone,bool aserms,bool abypass);
//
// Provide a useful default for splitting the quality between LDR and HDR.
extern void SplitQualityC(int totalquality,bool residuals,int &ldrquality,int &hdrquality);
///

///
#endif
