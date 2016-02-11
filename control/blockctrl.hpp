/*
** This class defines the minimal abstract interface the block based
** scan types require to access quantized data.
**
** $Id: blockctrl.hpp,v 1.8 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CONTROL_BLOCKCTRL_HPP
#define CONTROL_BLOCKCTRL_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class QuantizedRow;
class Scan;
///

/// class BlockCtrl
// This class defines the minimal abstract interface the block based
// scan types require to access quantized data.
class BlockCtrl : public JKeeper {
  //
public:
  BlockCtrl(class Environ *env)
    : JKeeper(env)
  { }
  //
  virtual ~BlockCtrl(void)
  { }
  //
  //
  // Return the current top MCU quantized line.
  virtual class QuantizedRow *CurrentQuantizedRow(UBYTE comp) = 0;
  //
  // Start a MCU scan by initializing the quantized rows for this row
  // in this scan.
  virtual bool StartMCUQuantizerRow(class Scan *scan) = 0;
  //
  // Make sure to reset the block control to the
  // start of the scan for the indicated components in the scan, 
  // required after collecting the statistics for this scan.
  virtual void ResetToStartOfScan(class Scan *scan) = 0;
  //
};
///

///
#endif
