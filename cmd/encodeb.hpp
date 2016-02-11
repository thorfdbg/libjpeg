/*
** Parameter definition and encoding for profile B.
**
** $Id: encodeb.hpp,v 1.20 2016/01/22 12:15:13 thor Exp $
**
*/

#ifndef CMD_ENCODEB_HPP
#define CMD_ENCODEB_HPP

/// Includes
#include "interface/types.hpp"
#include "std/stdio.hpp"
#if ISO_CODE
///

/// Prototypes
extern void EncodeB(const char *source,const char *ldr,const char *target,
		    double exposure,double autoexposure,double gamma,
		    double epsnum,double epsdenum,bool median,int smooth,
		    bool linearres,
		    int quality,int hdrquality,
		    int tabletype,int residualtt,
		    int colortrafo,bool progressive,bool rprogressive,
		    int hiddenbits,int residualhiddenbits,bool optimize,
		    bool openloop,bool deadzone,bool noclamp,const char *sub,const char *resub,
		    const char *alpha,int alphamode,int matte_r,int matte_g,int matte_b,
		    bool alpharesiduals,int alphaquality,int alphahdrquality,
		    int alphatt,int residualalphatt,
		    int ahiddenbits,int ariddenbits,int aresprec,
		    bool aopenloop,bool adeadzone,bool aserms,bool abypass);
//
extern void SplitQualityB(int splitquality,int &quality,int &hdrquality);
///

///
#endif
#endif
