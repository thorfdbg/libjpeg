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
** This class parses the markers and holds the decoder together.
**
** $Id: encoder.hpp,v 1.25 2015/03/24 09:45:31 thor Exp $
**
*/

#ifndef CODESTREAM_ENCODER_HPP
#define CODESTREAM_ENCODER_HPP

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
///

/// Forwards
class ByteStream;
class Quantization;
class HuffmanTable;
class Frame;
class Tables;
class Scan;
class Image;
struct JPG_TagItem;
///

/// class Encoder
// The default encoder. Nothing fancy, just
// uses the default tables and the default quantization
// settings. More to come.
class Encoder : public JKeeper {
  //
  // The image containing all the data
  class Image        *m_pImage;
  //
  // Create from a set of parameters the proper scan type.
  // This fills in the scan type of the base image and the residual image,
  // the number of refinement scans in the LDR and HDR domain, the
  // frame precision (excluding hidden/refinement bits) in the base and extension layer
  // and the number of additional precision bits R_b in the spatial domain.
  void FindScanTypes(const struct JPG_TagItem *tags,LONG frametype,UBYTE defaultdepth,
                     ScanType &scantype,ScanType &restype,
                     UBYTE &hiddenbits,UBYTE &riddenbits,
                     UBYTE &ldrprecision,UBYTE &hdrprecision,
                     UBYTE &rangebits) const;
  //
public:
  Encoder(class Environ *env);
  //
  ~Encoder(void);
  //
  // Create an image from the layout specified in the tags. See interface/parameters
  // for the available tags.
  class Image *CreateImage(const struct JPG_TagItem *tags);
};
///

///
#endif
