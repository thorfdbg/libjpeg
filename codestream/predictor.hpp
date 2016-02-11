/*
**
** This is the abstract description for the predictor of the JPEG
** predictive scan types. They implement, templated by the prediction
** mode, the various lookup types from neighbours.
**
** $Id: predictor.hpp,v 1.2 2014/11/17 12:15:25 thor Exp $
**
*/

#ifndef CODESTREAM_PREDICTOR_HPP
#define CODESTREAM_PREDICTOR_HPP
#if ACCUSOFT_CODE

/// Includes
#include "tools/environment.hpp"
#include "codestream/predictorbase.hpp"
///

/// class Predictor
// This is a templated class that performs the prediction from
// neighbouring samples for the predictive lossless modes.
template<PredictorBase::PredictionMode mode,int preshift>
class Predictor : public PredictorBase {
  //
  // The neutral grey value, if there is one.
  LONG m_lNeutral;
  //
public:
  Predictor(ULONG neutral = 0)
    : m_lNeutral(neutral)
  { }
  //
  ~Predictor(void)
  { }
  //
  // Predict a sample value depending on the prediction mode.
  // lp is the pointer to the current line, pp the one to the previous line.
  virtual LONG DecodeSample(LONG v,const LONG *lp,const LONG *pp) const
  {
    switch(mode) {
    case PredictorBase::None:
      // This is not a bug, must be signed 16 bit WORD for proper handling of differentials.
      return WORD(v) << preshift; // cast to signed, value is used directly.
    case PredictorBase::Left: // predict from left
      return UWORD(v + (lp[-1] >> preshift)) << preshift;
    case PredictorBase::Top: // predict from top
      return UWORD(v + (pp[0] >> preshift)) << preshift;
    case PredictorBase::LeftTop: // predict from left-top
      return UWORD(v + (pp[-1] >> preshift)) << preshift;
    case PredictorBase::Linear: // linear interpolation
      return UWORD(v + (lp[-1]  >> preshift) + (pp[0]  >> preshift) - (pp[-1] >> preshift)) 
	<< preshift;
    case PredictorBase::WeightA: // linear interpolation with weight on A
      return UWORD(v + (lp[-1]  >> preshift) + (((pp[0]  >> preshift) - 
						 (pp[-1] >> preshift)) >> 1)) << preshift;
    case PredictorBase::WeightB: // linear interpolation with weight on B
      return UWORD(v + (pp[0]  >> preshift) + (((lp[-1]  >> preshift) - 
						(pp[-1] >> preshift)) >> 1)) << preshift;
    case PredictorBase::Diagonal: // Only between A and B
      return UWORD(v + (((lp[-1] >> preshift) + (pp[0] >> preshift)) >> 1)) << preshift;
    case PredictorBase::Neutral:
      return UWORD(v + m_lNeutral) << preshift;
    }
	// Code should not go here.
	return m_lNeutral << preshift;
  }
  // 
  // Compute a symbol to encode from its value and its prediction based on the
  // prediction mode and the previous lines.
  virtual LONG EncodeSample(const LONG *lp,const LONG *pp) const
  {
    switch(mode) {
    case PredictorBase::None:
      return WORD(lp[0] >> preshift); // cast to signed, value is used directly.
    case PredictorBase::Left: // predict from left
      return WORD((lp[0] >> preshift) - (lp[-1] >> preshift));
    case PredictorBase::Top: // predict from top
      return WORD((lp[0] >> preshift) - (pp[0] >> preshift));
    case PredictorBase::LeftTop: // predict from left-top
      return WORD((lp[0] >> preshift) - (pp[-1] >> preshift));
    case PredictorBase::Linear: // linear interpolation
      return WORD((lp[0] >> preshift) - (lp[-1]  >> preshift) - (pp[0]  >> preshift) + (pp[-1] >> preshift));
    case PredictorBase::WeightA: // linear interpolation with weight on A
      return WORD((lp[0] >> preshift) - (lp[-1]  >> preshift) - (((pp[0]  >> preshift) - 
						 (pp[-1] >> preshift)) >> 1));
    case PredictorBase::WeightB: // linear interpolation with weight on B
      return WORD((lp[0] >> preshift) - (pp[0]  >> preshift) - (((lp[-1]  >> preshift) - 
						(pp[-1] >> preshift)) >> 1));
    case PredictorBase::Diagonal: // Only between A and B
      return WORD((lp[0] >> preshift) - (((lp[-1] >> preshift) + (pp[0] >> preshift)) >> 1));
    case PredictorBase::Neutral:
      return WORD((lp[0] >> preshift) - m_lNeutral);
    }
	return 0; // Code should not go here.
  }
};
///

///
#endif
#endif
