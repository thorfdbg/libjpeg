/*
** A JPEG LS scan covering only a single component.
**
** $Id: singlecomponentlsscan.hpp,v 1.8 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CODESTREAM_SINGLECOMPONENTLSSCAN_HPP
#define CODESTREAM_SINGLECOMPONENTLSSCAN_HPP

/// Includes
#include "codestream/jpeglsscan.hpp"
///

/// class SingleComponentLSScan
class SingleComponentLSScan : public JPEGLSScan {
  //
  //
public:
  // Create a new scan. This is only the base type.
  SingleComponentLSScan(class Frame *frame,class Scan *scan,UBYTE near,const UBYTE *mapping,UBYTE point);
  //
  virtual ~SingleComponentLSScan(void);
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

