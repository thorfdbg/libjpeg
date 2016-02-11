/*
**
** This is the base class for all predictive scan types, it provides the
** services useful to implement them such that the derived classes can
** focus on the actual algorithm.
**
** $Id: predictivescan.cpp,v 1.14 2015/03/25 08:45:43 thor Exp $
**
*/

/// Includes
#include "codestream/predictivescan.hpp"
#include "io/bytestream.hpp"
#include "marker/frame.hpp"
#include "marker/scan.hpp"
#include "marker/component.hpp"
#include "codestream/tables.hpp"
#include "codestream/predictorbase.hpp"
#include "tools/line.hpp"
///

/// PredictiveScan::PredictiveScan
PredictiveScan::PredictiveScan(class Frame *frame,class Scan *scan,UBYTE predictor,UBYTE lowbit,bool differential)
  : EntropyParser(frame,scan)
#if ACCUSOFT_CODE
  , m_pLineCtrl(NULL), m_ucPredictor(predictor), m_ucLowBit(lowbit),
    m_bDifferential(differential)
#endif
{ 
#if ACCUSOFT_CODE
  m_ucCount = scan->ComponentsInScan();
  memset(m_pPredictors ,0,sizeof(m_pPredictors));
  memset(m_pPredict    ,0,sizeof(m_pPredict));
  memset(m_pLinePredict,0,sizeof(m_pLinePredict));
#else
  NOREF(predictor);
  NOREF(lowbit);
  NOREF(differential);
#endif
}
///

/// PredictiveScan::~PredictiveScan
PredictiveScan::~PredictiveScan(void)
{
#if ACCUSOFT_CODE
  for(int i = 0;i < 4;i++) {
    delete m_pPredictors[i];
  }
#endif
}
///

/// PredictiveScan::FindComponentDimensions
// Collect the component information.
void PredictiveScan::FindComponentDimensions(void)
{ 
#if ACCUSOFT_CODE
  int i;

  m_ulPixelWidth  = m_pFrame->WidthOf();
  m_ulPixelHeight = m_pFrame->HeightOf();

  if (m_pPredictors[0] == NULL) {
    PredictorBase::CreatePredictorChain(m_pEnviron,m_pPredictors,
					(m_bDifferential)?(PredictorBase::None):
					(PredictorBase::PredictionMode(m_ucPredictor)),
					FractionalColorBitsOf() + m_ucLowBit,(1L << m_pFrame->PrecisionOf()) >> 1);
  }

  for(i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    UBYTE subx            = comp->SubXOf();
    UBYTE suby            = comp->SubYOf();
    
    m_ulWidth[i]          = (m_ulPixelWidth  + subx - 1) / subx;
    m_ulHeight[i]         = (m_ulPixelHeight + suby - 1) / suby; 
    m_ucMCUWidth[i]       = comp->MCUWidthOf();
    m_ucMCUHeight[i]      = comp->MCUHeightOf();
    m_ulX[i]              = 0;
    m_ulY[i]              = 0;
    m_pPredict[i]         = m_pPredictors[0]; // always start with the top-left predictor.
    m_pLinePredict[i]     = m_pPredictors[0];
  }

  if (m_ucCount == 1) {
    m_ucMCUWidth[0]  = 1;
    m_ucMCUHeight[0] = 1;
  }
#endif 
}
///


/// PredictiveScan::ClearMCU
// Clear the entire MCU
void PredictiveScan::ClearMCU(struct Line **top)
{ 
#if ACCUSOFT_CODE
  for(int i = 0;i < m_ucCount;i++) {
    class Component *comp = ComponentOf(i);
    struct Line *line     = top[i];
    UBYTE ym              = comp->MCUHeightOf();
    LONG neutral          = ((1L << m_pFrame->PrecisionOf()) >> 1) << FractionalColorBitsOf();
    //
    do {
      LONG *p = line->m_pData;
      LONG *e = line->m_pData + m_ulWidth[i];
      do {
	*p = neutral;
      } while(++p < e);

      if (line->m_pNext)
	line = line->m_pNext;
    } while(--ym);
  }
#else
  NOREF(top);
#endif
}
///

/// PredictiveScan::Flush
// Flush at the end of a restart interval
// when writing out code. Reset predictors, check
// for the correctness of the restart alignment.
void PredictiveScan::FlushOnMarker(void)
{
#if ACCUSOFT_CODE
  int i;
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_ulX[i]) {
      JPG_WARN(MALFORMED_STREAM,"LosslessScan::Flush",
	       "found restart marker in the middle of the line, expect corrupt results");
      break;
    }
    // Restart prediction from top-left.
    m_pPredict[i]     = m_pPredictors[0];
    m_pLinePredict[i] = m_pPredictors[0];
  } 
#endif
}
///

/// PredictiveScan::Restart
// Restart after reading a full restart interval,
// reset the predictors, check for the correctness
// of the restart interval.
void PredictiveScan::RestartOnMarker(void)
{
#if ACCUSOFT_CODE
  int i;
  
  for(i = 0;i < m_ucCount;i++) {
    if (m_ulX[i]) {
      JPG_WARN(MALFORMED_STREAM,"LosslessScan::Restart",
	       "found restart marker in the middle of the line, expect corrupt results");
      break;
    }
    // Restart prediction from top-left.
    m_pPredict[i]     = m_pPredictors[0];
    m_pLinePredict[i] = m_pPredictors[0];
  }
#endif
}
///
