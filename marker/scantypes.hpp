/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/
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
