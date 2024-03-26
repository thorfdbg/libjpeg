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
** This class is the abstract base class for all boxes, the generic
** extension mechanism for 10918-1. Boxes are used consistently for all
** types of extended data.
**
** $Id: box.hpp,v 1.18 2024/03/25 18:42:06 thor Exp $
**
*/

#ifndef BOXES_BOX_HPP
#define BOXES_BOX_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
///

/// Defines
// Create a box ID from an ASCII code.
#define MAKE_ID(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | ((d) << 0))
///

/// Forwards
class DecoderStream;
class MemoryStream;
class ByteStream;
class SuperBox;
class NameSpace;
class Tables;
///

/// class Box
// This is the base class for all types of extended data,
// using the generic APP11 marker to fit its data into the
// syntax of 10918-1.
class Box : public JKeeper {
  friend class SuperBox;
  //
  // Boxes are queued in the table class, this is the next
  // box in the list.
  class Box           *m_pNext;
  //
  // The box type, the four-character identifyer.
  ULONG                m_ulBoxType;
  //
  // The size of the box. The standard currently only allows box sizes up
  // to 4GB, though be prepared that this will grow beyond this limit
  // using the same mechanism as in J2K.
  UQUAD                m_uqBoxSize;
  //
  // Number of bytes already parsed off in the decoder stream. As soon
  // as this equals the indicated box size, the second level parsing
  // of the box contents may begin.
  UQUAD                m_uqParsedBytes;
  //
  // The enumerator that disambuigates between boxes of the same box
  // type.
  UWORD                m_usEnumerator;
  //
  // The stream that keeps the still unparsed data until the box is complete.
  class DecoderStream *m_pInputStream;
  //
  // Output stream when generating the box at the encoder side.
  class MemoryStream  *m_pOutputStream;
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box. Returns true in case the contents is
  // parsed and the stream can go away.
  virtual bool ParseBoxContent(class ByteStream *stream,UQUAD boxsize) = 0;
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  // Returns whether the box content is already complete and the stream
  // can go away.
  virtual bool CreateBoxContent(class MemoryStream *target) = 0;
  //
  // Write the box content into a superbox. This does not require an enumerator.
  void WriteBoxContent(class ByteStream *target);
  //
protected:
  //
  // Create the input stream we can parse from, or return it.
  class DecoderStream *InputStreamOf(void);
  //
  // Create the output stream we can write into, or return it.
  class MemoryStream *OutputStreamOf(void);
  //
  // Write the box contents to the given stream, potentially breaking it up into
  // several APP11 markers.
  void WriteBoxContent(class ByteStream *target,UWORD enu);
  //
public:
  Box(class Environ *env,class Box *&boxlist,ULONG boxtype);
  //
  virtual ~Box(void);
  //
  // Parse an APP11 extended JPEG marker, find or create the necessary box, and
  // append the data there. This is the first level parsing.
  // As soon as the box is complete, perform second level parsing. Requires the 
  // head of the box list as the box is possibly created.
  // The marker, the marker size and the common identifier are already parsed off.
  // Returns the box as soon as it can be delivered, i.e. all bytes are available.
  static class Box *ParseBoxMarker(class Tables *tables,class Box *&boxlist,class ByteStream *stream,UWORD length);
  //
  // Write all boxes into APP11 markers, breaking them up and creating the
  // enumerators for them. This first calls second level box creation, then writes
  // the data into the output stream.
  static void WriteBoxMarkers(class Box *&boxlist,class ByteStream *target);
  //
  // Type of this box.
  ULONG BoxTypeOf(void) const
  {
    return m_ulBoxType;
  }
  //
  // Return the index of this box within the total list of boxes.
  UWORD EnumeratorOf(void) const
  {
    return m_usEnumerator;
  }
  //
  // Create a box of the given type and return it.
  static class Box *CreateBox(class Tables *tables,class Box *&boxlist,ULONG type);
  //
  // Return the next box in the box chain.
  class Box *NextOf(void) const
  {
    return m_pNext;
  }
  //
  // Return whether all data of this box has been received
  // already.
  bool isComplete(void) const
  {
    if (m_uqParsedBytes >= m_uqBoxSize)
      return true;
    return false;
  }
};
///

///
#endif
