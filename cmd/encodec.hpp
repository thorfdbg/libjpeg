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
