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
** This box implements a data container for refinement or residual 
** scans, as hidden in the APP11 markers.
**
** $Id: databox.hpp,v 1.10 2015/03/18 11:54:12 thor Exp $
**
*/

#ifndef BOXES_DATABOX_HPP
#define BOXES_DATABOX_HPP

/// Includes
#include "boxes/box.hpp"
///

/// Forwards
class ByteStream;
///

/// class DataBox
// This box implements a data container for refinement scans, as hidden
// in the APP11 markers.
class DataBox : public Box {
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box.
  virtual bool ParseBoxContent(class ByteStream *,UQUAD)
  {
    // Actually, this does nothing here. The data just remains in the
    // decoder stream so the actual decoder can grab it when needed.
    return false;
  }
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  // Returns the buffer where the data is in - the box may use its own buffer.
  virtual bool CreateBoxContent(class MemoryStream *)
  {
    // This does actually nothing here. Writing the box is triggered
    // when required, but this is not part of the typical box writing logic.
    return false;
  }
  //
public:
  //
  // Data boxes carry plenty of raw data as a stream. Here are
  // their types.
  enum {
    ResidualType                = MAKE_ID('R','E','S','I'), // residual codestream
    RefinementType              = MAKE_ID('F','I','N','E'), // legacy refinement
    ResidualRefinementType      = MAKE_ID('R','F','I','N'), // refinement of the residual codestream
    AlphaType                   = MAKE_ID('A','L','F','A'), // alpha codestream
    AlphaRefinementType         = MAKE_ID('A','F','I','N'), // refinement of the alpha codestream
    AlphaResidualType           = MAKE_ID('A','R','E','S'), // alpha residual codestream
    AlphaResidualRefinementType = MAKE_ID('A','R','R','F')  // Aruff,wuff,wuff.... LeChuck... grrr.... Alpha channel residual refinement box
  };
  //
  // Create a refinement data box.
  DataBox(class Environ *env,class Box *&boxlist,ULONG type)
    : Box(env,boxlist,type)
  { }
  //
  //
  virtual ~DataBox(void)
  { }
  //
  // Return the byte stream buffer where the encoder can drop the data
  // on encoding.
  class ByteStream *EncoderBufferOf(void);
  //
  // Return the stream the decoder will decode from.
  class ByteStream *DecoderBufferOf(void);
  //
  // Flush the buffered data of the box and create the markers.
  // En is an enumerator that disambigutes identical boxes.
  void Flush(class ByteStream *target,UWORD en);
};
///

///
#endif
