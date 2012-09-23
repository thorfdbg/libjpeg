/*************************************************************************
** Copyright (c) 2011-2012 Accusoft                                     **
** This program is free software, licensed under the GPLv3              **
** see README.license for details                                       **
**									**
** For obtaining other licenses, contact the author at                  **
** thor@math.tu-berlin.de                                               **
**                                                                      **
** Written by Thomas Richter (THOR Software)                            **
** Sponsored by Accusoft, Tampa, FL and					**
** the Computing Center of the University of Stuttgart                  **
**************************************************************************

This software is a complete implementation of ITU T.81 - ISO/IEC 10918,
also known as JPEG. It implements the standard in all its variations,
including lossless coding, hierarchical coding, arithmetic coding and
DNL, restart markers and 12bpp coding.

In addition, it includes support for new proposed JPEG technologies that
are currently under discussion in the SC29/WG1 standardization group of
the ISO (also known as JPEG). These technologies include lossless coding
of JPEG backwards compatible to the DCT process, and various other
extensions.

The author is a long-term member of the JPEG committee and it is hoped that
this implementation will trigger and facilitate the future development of
the JPEG standard, both for private use, industrial applications and within
the committee itself.

  Copyright (C) 2011-2012 Accusoft, Thomas Richter <thor@math.tu-berlin.de>

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
** $Id: scantypes.hpp,v 1.5 2012-09-23 14:10:13 thor Exp $
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
  Residual, // New scan types
  ACResidual,
  JPEG_LS // not part of 10918, but handled here as well.
};

#endif
