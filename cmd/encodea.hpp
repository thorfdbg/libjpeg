/*
** Parameter definition and encoding for profile A.
**
** $Id: encodea.hpp,v 1.15 2016/01/22 12:15:13 thor Exp $
**
*/

#ifndef CMD_ENCODEA_HPP
#define CMD_ENCODEA_HPP

/// Includes
#include "interface/types.hpp"
#include "std/stdio.hpp"
#if ISO_CODE
///

/// Prototypes
// Encode an image in profile A, filling in all the parameters the codec needs.
extern void EncodeA(const char *source,const char *ldrsource,const char *target,
		    int quality,int hdrquality,
		    int tabletype,int residualtt,int colortrafo,
		    bool progressive,bool rprogressive,
		    int hiddenbits,int residualhiddenbits,bool optimize,
		    bool openloop,bool deadzone,bool noclamp,const char *sub,const char *resub,
		    double gamma,bool median,int smooth,
		    const char *alpha,int alphamode,int matte_r,int matte_g,int matte_b,
		    bool alpharesiduals,int alphaquality,int alphahdrquality,
		    int alphatt,int residualalphatt,
		    int ahiddenbits,int ariddenbits,int aresprec,
		    bool aoopenloop,bool adeadzone,bool aserms,bool abypass);
//
// Provide a useful default for splitting the quality between LDR and HDR.
extern void SplitQualityA(int splitquality,int &quality,int &hdrquality);
///

///
#endif
#endif
