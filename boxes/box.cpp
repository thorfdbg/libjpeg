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
** $Id: box.cpp,v 1.29 2024/03/25 18:42:06 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "codestream/tables.hpp"
#include "io/bytestream.hpp"
#include "io/decoderstream.hpp"
#include "io/memorystream.hpp"
#include "boxes/databox.hpp"
#include "boxes/mergingspecbox.hpp"
#include "boxes/inversetonemappingbox.hpp"
#include "boxes/floattonemappingbox.hpp"
#include "boxes/parametrictonemappingbox.hpp"
#include "boxes/lineartransformationbox.hpp"
#include "boxes/checksumbox.hpp"
#include "boxes/filetypebox.hpp"
#include "boxes/alphabox.hpp"
#include "std/assert.hpp"
///

/// Box::Box
// Creates a new box, data still unknown
Box::Box(class Environ *env,class Box *&boxlist,ULONG type)
  : JKeeper(env), m_pNext(boxlist), m_ulBoxType(type), m_uqBoxSize(0), m_uqParsedBytes(0), 
    m_pInputStream(NULL), m_pOutputStream(NULL)
{ 
  boxlist = this;
}
///

/// Box::~Box
// Releases the box and all its components.
Box::~Box(void)
{
  delete m_pInputStream;
  delete m_pOutputStream;
}
///

/// Box::ParseBoxMarker
// Parse an APP11 extended JPEG marker, find or create the necessary box, and
// append the data there. This is the first level parsing.
// As soon as the box is complete, perform second level parsing. Requires the 
// head of the box list as the box is possibly created.
// The marker, the marker size and the common identifier are already parsed off.
class Box *Box::ParseBoxMarker(class Tables *tables,class Box *&boxlist,class ByteStream *stream,UWORD length)
{
  class Environ *m_pEnviron = tables->EnvironOf(); // borrow the environment from the tables.
  class Box *box;
  UWORD en;
  UWORD blen; // payload data size
  ULONG z;
  UQUAD lbox;
  ULONG tbox;
  LONG dt;
  LONG overhead = 2 + 2 + 2 + 4 + 4 + 4;

  // Must include LE,CI,En,Z,LBox and TBox
  if (length <= overhead)
    JPG_THROW(MALFORMED_STREAM,"Box::ParseBoxMarker","JPEG stream is malformed, "
              "APP11 extended box marker size is too short.");

  en    = stream->GetWord();       // the enumerator.
  z     = stream->GetWord() << 16; // the sequence number.
  z    |= stream->GetWord();
  lbox  = stream->GetWord() << 16;
  lbox |= stream->GetWord();
  blen  = length - overhead;
  // This field can be either one, in which case an LBox field is present (though not yet documented in the specs)
  // or it can be larger or equal than eight, in which the box is regular. The values zero (box size not known)
  // and all other values are not valid here.
  if (lbox != 1 && lbox < 8)
    JPG_THROW(MALFORMED_STREAM,"Box::ParseBoxMarker","JPEG stream is malformed, "
              "box length field is invalid");
  tbox  = stream->GetWord() << 16;
  dt    = stream->GetWord();
  if (dt == ByteStream::EOF)
    JPG_THROW(UNEXPECTED_EOF,"Box::ParseBoxMarker","JPEG stream is malformed, unexpected "
              "end of file while parsing an APP11 marker");
  tbox |= dt;
  //
  // Try to read the XLBox field if there is one.
  if (lbox == 1) {
    overhead += 8;
    if (length <= overhead)
      JPG_THROW(MALFORMED_STREAM,"Box::ParseBoxMarker","JPEG stream is malformed, "
                "APP11 extended box marker size is too short.");
    lbox  = UQUAD(stream->GetWord()) << 48;
    lbox |= UQUAD(stream->GetWord()) << 32;
    lbox |= UQUAD(stream->GetWord()) << 16;
    dt    = stream->GetWord();
    if (dt == ByteStream::EOF)
      JPG_THROW(UNEXPECTED_EOF,"Box::ParseBoxMarker","JPEG stream is malformed, unexpected "
                "end of file while parsing an APP11 marker");
    if (lbox < 8 + 8)
      JPG_THROW(MALFORMED_STREAM,"Box::ParseBoxMarker","JPEG stream is malformed, "
                "box length field is invalid");
    lbox |= dt;
    blen -= 8; // less payload then.
    lbox -= 8; // the box size increased by XLBox.
  }
  lbox -= 8; // Remove the box length and type from the payload size.
  //
  // Check whether there is already a box with the same enumerator and box type. If so, append.
  for(box = boxlist;box;box = box->m_pNext) {
    if (box->m_ulBoxType == tbox && box->m_usEnumerator == en) {
      // This box will fit. Check whether the box size is consistent.
      if (box->m_uqBoxSize != lbox)
        JPG_THROW(MALFORMED_STREAM,"Box::ParseBoxMarker","JPEG stream is malformed, "
                  "box size is not consistent across APP11 markers");
      if (box->m_pInputStream == NULL)
        JPG_THROW(MALFORMED_STREAM,"Box::ParseBoxMarker","JPEG stream is malformed, "
                  "received box data beyond box length");
      break;
    }
  }
  //
  // If no valid box has been found so far, create now a new one.
  assert(box == NULL || box->InputStreamOf() != stream);
  if (box == NULL) {
    // This is the "virtual constructor"
    box                 = CreateBox(tables,boxlist,tbox);
    // If the box could not be created because the box type is 
    // unknown, just skip it.
    if (box == NULL) {
      stream->SkipBytes(blen);
      return NULL;
    }
    //
    box->m_ulBoxType    = tbox;
    box->m_uqBoxSize    = lbox;
    box->m_usEnumerator = en;
  }
  //
  // Add the data to the input stream.
  box->InputStreamOf()->Append(stream,blen,z);
  box->m_uqParsedBytes += blen;
  //
  if (box->m_uqParsedBytes > box->m_uqBoxSize)
    JPG_THROW(MALFORMED_STREAM,"Box::ParseBoxMarker","JPEG stream is invalid, more data in the application "
              "marker than indicated and required by the box contained within.");
  //
  // If the box is complete, start its second level parsing.
  if (box->m_uqParsedBytes == box->m_uqBoxSize) {
    if (box->ParseBoxContent(box->InputStreamOf(),box->m_uqBoxSize)) {
      // Contents is parsed.
      delete box->m_pInputStream;
      box->m_pInputStream = NULL;
    }
    // Even if the box does not want to be parsed, deliver it because it is
    // complete enough.
    return box;
  }
  return NULL;
}
///


/// Box::WriteBoxContent
// Write the box content into a superbox, without breaking it up and without
// requiring an enumerator.
void Box::WriteBoxContent(class ByteStream *target)
{  
  assert(m_pOutputStream);
  class MemoryStream readback(m_pEnviron,m_pOutputStream,JPGFLAG_OFFSET_BEGINNING);
  UQUAD lbox = m_uqBoxSize = m_pOutputStream->BufferedBytes();
  UQUAD wl   = lbox;
  //
  // Add additional overhead for writing the marker, which are the le,CI,EN,Z,Lbox and Tbox fields,
  // plus potentially XLBox.
  lbox       += 4 + 4; // the box length and type are part of the box length.
  if (lbox > MAX_ULONG) {
    lbox     += 8; // The XLBox field is part of the box length.
  }
  //
  // Write LBox
  if (lbox > MAX_ULONG) { // actually, this cannot happen as the memory stream only buffers 2^32 bytes, though...
    target->PutWord(0);
    target->PutWord(1);
  } else {
    target->PutWord(lbox >> 16);
    target->PutWord(lbox);
  }
  //
  // Write tbox
  target->PutWord(m_ulBoxType >> 16);
  target->PutWord(m_ulBoxType);
  // write XLBox if required.
  if (lbox > MAX_ULONG) {
    target->PutWord(lbox >> 48);
    target->PutWord(lbox >> 32);
    target->PutWord(lbox >> 16);
    target->PutWord(lbox);
  }
  //
  // Write the payload data.
  while(wl > 0) {
    ULONG write;
    //
    if (wl > MAX_ULONG) {
      write = MAX_ULONG;
    } else {
      write = wl;
    }
    //
    readback.Push(target,wl);
    //
    // And go for the next round, there may be several markers.
    wl -= write;
  }
  //
  // Done with it.
  delete m_pOutputStream;
  m_pOutputStream = NULL;
}
///

/// Box::WriteBoxContent
// Write the box contents to the given stream, potentially breaking it up into
// several APP11 markers.
void Box::WriteBoxContent(class ByteStream *target,UWORD en)
{    
  assert(m_pOutputStream);
  class MemoryStream readback(m_pEnviron,m_pOutputStream,JPGFLAG_OFFSET_BEGINNING);
  UQUAD lbox = m_uqBoxSize = m_pOutputStream->BufferedBytes();
  UQUAD wl   = lbox;
  ULONG z    = 1; // FIX: Z starts with 1.
  ULONG overhead;
  //
  // Add additional overhead for writing the marker, which are the le,CI,EN,Z,Lbox and Tbox fields,
  // plus potentially XLBox.
  overhead    = 2+2+2+4+4+4;
  lbox       += 4 + 4; // the box length and type are part of the box length.
  if (lbox > MAX_ULONG) {
    overhead += 8;
    lbox     += 8; // The XLBox field is part of the box length.
  }
  //
  while(wl) {
    ULONG markersize;
    //
    if (wl > MAX_LONG) {
      markersize = MAX_LONG;
    } else {
      markersize = wl;
    }
    //
    // Clip to a maximum of 64K.
    markersize += overhead;
    if (markersize > MAX_UWORD)
      markersize = MAX_UWORD;
    //
    target->PutWord(0xffeb); // AP11 marker
    target->PutWord(markersize);
    target->PutWord(0x4a50); // The common identifier
    target->PutWord(en);     // The enumerator
    target->PutWord(z >> 16);
    target->PutWord(z);
    if (lbox > MAX_ULONG) { // actually, this cannot happen as the memory stream only buffers 2^32 bytes, though...
      target->PutWord(0);
      target->PutWord(1);
    } else {
      target->PutWord(lbox >> 16);
      target->PutWord(lbox);
    }
    target->PutWord(m_ulBoxType >> 16);
    target->PutWord(m_ulBoxType);
    if (lbox > MAX_ULONG) {
      target->PutWord(lbox >> 48);
      target->PutWord(lbox >> 32);
      target->PutWord(lbox >> 16);
      target->PutWord(lbox);
    }
    //
    // Write the payload data.
    readback.Push(target,markersize - overhead);
    //
    // And go for the next round, there may be several markers.
    wl -= markersize - overhead;
    z++;
    if (z == 0)
      JPG_THROW(OVERFLOW_PARAMETER,"Box::WriteBoxContent","Cannot create JPEG stream, box contents is too large");
  }
  //
  // Done with it.
  delete m_pOutputStream;
  m_pOutputStream = NULL;
}
///

/// Box::WriteBoxMarkers
// Write all boxes into APP11 markers, breaking them up and creating the
// enumerators for them. This first calls second level box creation, then writes
// the data into the output stream.
void Box::WriteBoxMarkers(class Box *&boxlist,class ByteStream *target)
{
  class Environ *m_pEnviron = target->EnvironOf();
  class Box *box,*ebox;
  UWORD en;

  // Step 1: Bring the boxes into the correct order. The file type box - if present -
  // must go first.
  for(box = boxlist;box;box = box->m_pNext) {
    if (box->m_pNext && box->m_pNext->BoxTypeOf() == FileTypeBox::Type) {
      class Box *ftype = box->m_pNext;
      // Remove from its current position, and link in first.
      box->m_pNext     = ftype->m_pNext;
      ftype->m_pNext   = boxlist;
      boxlist          = ftype;
      break;
    }
  }
  //
  // Step 2: Compute the enumerators.
  for(box = boxlist;box;box = box->m_pNext) {
    // Check whether there are other boxes of the same type. If so, 
    // increment the enumerator to find a unique one.
    for(ebox = boxlist,en = 1;ebox != box;ebox = ebox->m_pNext) {
      if (ebox->m_ulBoxType == box->m_ulBoxType) {
        en = ebox->m_usEnumerator + 1;
        if (en == 0)
          JPG_THROW(OVERFLOW_PARAMETER,"Box::WriteBoxMarkers","Cannot create JPEG stream, too many boxes of the "
                    "same type present");
      }
    }
    //
    // Fill in the enumerator for later reference.
    box->m_usEnumerator = en;
    //
    // Perform second stage box creation. The box may choose to create or use its
    // own output stream.
    if (box->CreateBoxContent(box->OutputStreamOf())) {
      //
      // If the box is ready to generate its output, do this now.
      box->WriteBoxContent(target,en);
    }
  }
}
///


/// Box::CreateBox
// Create a box of the given type - this is something like a "virtual constructor".
// Return NULL in case the box type is not known.
class Box *Box::CreateBox(class Tables *tables,class Box *&boxlist,ULONG tbox)
{
  class Environ *m_pEnviron = tables->EnvironOf();
  
  switch(tbox) {
  case DataBox::ResidualType:
  case DataBox::RefinementType:
  case DataBox::ResidualRefinementType:
  case DataBox::AlphaType:
  case DataBox::AlphaRefinementType:
  case DataBox::AlphaResidualType:
  case DataBox::AlphaResidualRefinementType:
    return new(m_pEnviron) class DataBox(m_pEnviron,boxlist,tbox);
  case MergingSpecBox::SpecType:
    if (tables->ImageNamespace()->hasPrimaryLookup())
      JPG_THROW(OBJECT_EXISTS,"Box::CreateBox","found duplicate merging specification box");
    return new(m_pEnviron) class MergingSpecBox(tables,boxlist,tbox);
  case MergingSpecBox::AlphaType:
    if (tables->AlphaNamespace()->hasPrimaryLookup())
      JPG_THROW(OBJECT_EXISTS,"Box::CreateBox","found duplicate merging specification box");
    return new(m_pEnviron) class MergingSpecBox(tables,boxlist,tbox);
  case InverseToneMappingBox::Type:
    return new(m_pEnviron) class InverseToneMappingBox(m_pEnviron,boxlist);
  case FloatToneMappingBox::Type:
    return new(m_pEnviron) class FloatToneMappingBox(m_pEnviron,boxlist);
  case ParametricToneMappingBox::Type:
    return new(m_pEnviron) class ParametricToneMappingBox(m_pEnviron,boxlist);
  case LinearTransformationBox::Type:
    return new(m_pEnviron) class LinearTransformationBox(m_pEnviron,boxlist);
  case ChecksumBox::Type:
    return new(m_pEnviron) class ChecksumBox(m_pEnviron,boxlist);
  case FileTypeBox::Type:
    return new(m_pEnviron) class FileTypeBox(m_pEnviron,boxlist);
  }
  //
  // Unknown box types are ignored.
  return NULL;
}
///

/// Box::OutputStreamOf
// Create the output stream we can write into, or return it.
class MemoryStream *Box::OutputStreamOf(void)
{
  if (m_pOutputStream == NULL) {
      m_pOutputStream = new(m_pEnviron) class MemoryStream(m_pEnviron);
  }

  return m_pOutputStream;
}
///

/// Box::InputStreamOf
// Create the input stream we can parse from, or return it.
class DecoderStream *Box::InputStreamOf(void)
{
  if (m_pInputStream == NULL) {
    m_pInputStream = new(m_pEnviron) class DecoderStream(m_pEnviron);
  }

  return m_pInputStream;
}
///
