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
** Parameter definition and encoding for profile C.
**
** $Id: encodec.hpp,v 1.18 2021/11/15 08:59:59 thor Exp $
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
                    int colortrafo,bool baseline,bool lossless,bool progressive,
                    bool residual,bool optimize,bool accoding,
                    bool rsequential,bool rprogressive,bool raccoding,
                    bool qscan,UBYTE levels,bool pyramidal,bool writednl,ULONG restart,
                    double gamma,
                    int lsmode,bool noiseshaping,bool serms,bool losslessdct,
                    bool openloop,bool deadzone,bool lagrangian,bool dering,
                    bool xyz,bool cxyz,
                    int hiddenbits,int riddenbits,int resprec,bool separate,
                    bool median,bool noclamp,int smooth,
                    bool dctbypass,
                    const char *sub,const char *ressub,
                    const char *alpha,int alphamode,int matte_r,int matte_g,int matte_b,
                    bool alpharesiduals,int alphaquality,int alphahdrquality,
                    int alphatt,int residualalphatt,
                    int ahiddenbits,int ariddenbits,int aresprec,
                    bool aopenloop,bool adeadzone,bool alagrangian,bool adering,
                    bool aserms,bool abypass,
                    const char *quantsteps,const char *residualquantsteps,
                    const char *alphasteps,const char *residualalphasteps);
//
// Provide a useful default for splitting the quality between LDR and HDR.
extern void SplitQualityC(int totalquality,bool residuals,int &ldrquality,int &hdrquality);
///

///
#endif
