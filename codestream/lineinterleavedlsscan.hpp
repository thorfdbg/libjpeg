/*
** A JPEG LS scan interleaving the components line by line.
**
** $Id: lineinterleavedlsscan.hpp,v 1.8 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CODESTREAM_LINEINTERLEAVEDLSSCAN_HPP
#define CODESTREAM_LINEINTERLEAVEDLSSCAN_HPP

/// Includes
#include "codestream/jpeglsscan.hpp"
///

/// class LineInterleavedLSScan
// A JPEG LS scan interleaving the components line by line.
class LineInterleavedLSScan : public JPEGLSScan {
  //
  //
public:
  // Create a new scan. This is only the base type.
  LineInterleavedLSScan(class Frame *frame,class Scan *scan,UBYTE near,const UBYTE *mapping,UBYTE point);
  //
  virtual ~LineInterleavedLSScan(void);
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

