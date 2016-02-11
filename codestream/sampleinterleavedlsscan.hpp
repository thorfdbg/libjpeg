/*
** A JPEG LS scan interleaving samples of several components,
** sample by sample.
**
** $Id: sampleinterleavedlsscan.hpp,v 1.9 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CODESTREAM_SAMPLEINTERLEAVEDLSSCAN_HPP
#define CODESTREAM_SAMPLEINTERLEAVEDLSSCAN_HPP

/// Includes
#include "codestream/jpeglsscan.hpp"
///

/// class SampleInterleavedLSScan
class SampleInterleavedLSScan : public JPEGLSScan {
  //
  // Collect component information and install the component dimensions.
  virtual void FindComponentDimensions(void);
  //
public:
  // Create a new scan. This is only the base type.
  SampleInterleavedLSScan(class Frame *frame,class Scan *scan,
			  UBYTE near,const UBYTE *mapping,UBYTE point);
  //
  virtual ~SampleInterleavedLSScan(void);
  // 
  // Parse a single MCU in this scan. Return true if there are more
  // MCUs in this row.
  virtual bool ParseMCU(void);
  //
  // Write a single MCU in this scan.
  virtual bool WriteMCU(void); 
};
///


///
#endif

