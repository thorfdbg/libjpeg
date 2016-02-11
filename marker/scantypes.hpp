/*
**
** Represents the scan including the scan header for the
** arithmetic coding procedure.
**
** $Id: scantypes.hpp,v 1.12 2015/02/25 16:09:42 thor Exp $
**
*/

#ifndef MARKER_SCANTYPES_HPP
#define MARKER_SCANTYPES_HPP

enum ScanType {
  Baseline, // also sequential
  Sequential,
  Progressive,
  Lossless,
  DifferentialSequential,
  DifferentialProgressive,
  DifferentialLossless,
  Dimensions,    // internal use only: This is the frame that holds the DHP marker.
  ACSequential,  // AC-coded sequential
  ACProgressive, // AC-coded progressive
  ACLossless,    // AC-coded lossless
  ACDifferentialSequential,
  ACDifferentialProgressive,
  ACDifferentialLossless,
  Residual,      // New scan types
  ACResidual,
  ResidualProgressive,
  ACResidualProgressive,
  ResidualDCT,   // HugeDCT scan type added in the Sydney meeting
  ACResidualDCT, // unofficial
  JPEG_LS // not part of 10918, but handled here as well.
};

#endif
