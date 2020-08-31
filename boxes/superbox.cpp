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
** This class provides the necessary mechanisms for superbox parsing.
** It describes a box that contains sub-boxes and parses the sub-boxes.
**
** $Id: superbox.cpp,v 1.6 2014/09/30 08:33:15 thor Exp $
**
*/

/// Includes
#include "interface/types.hpp"
#include "interface/parameters.hpp"
#include "tools/environment.hpp"
#include "boxes/box.hpp"
#include "boxes/superbox.hpp"
#include "boxes/namespace.hpp"
#include "io/memorystream.hpp"
#include "io/decoderstream.hpp"
///

/// SuperBox::SuperBox
SuperBox::SuperBox(class Environ *env,class Box *&boxlist,ULONG boxtype)
      : Box(env,boxlist,boxtype), m_pSubBoxes(NULL)
{
}
///

/// SuperBox::~SuperBox
SuperBox::~SuperBox(void)
{
  class Box *box;

  // Destroy the subboxes recursively.
  while((box = m_pSubBoxes)) {
    m_pSubBoxes = box->NextOf();
    delete box;
  }
}
///

/// SuperBox::RegisterNameSpace
// Register this box as primary namespace.
void SuperBox::RegisterNameSpace(class NameSpace *names)
{
  names->DefinePrimaryLookup(&m_pSubBoxes);
}
///

/// SuperBox::ParseBoxContent
// Parse the contents of the superbox as sub-boxes. This is implemented here
// because the contents is already structured at the box-level. It
// creates boxes from the box types, but leaves the actual box-parsing to the
// correspondig super-box implementation.
bool SuperBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  //
  // Superboxes may be empty.
  while(boxsize > 0) {
    class Box *box;
    LONG  lo,hi;
    ULONG lbox;
    ULONG tbox;
    ULONG overhead = 0;
    UQUAD xlbox;
    // At least the LBox and the TBox fields must be present.
    if (boxsize < 4 + 4)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::ParseBoxContent","found incomplete box header within a superbox");
    //
    // Parse off lbox.
    hi   = stream->GetWord();
    lo   = stream->GetWord();
    if (lo == ByteStream::EOF)
      JPG_THROW(UNEXPECTED_EOF,"SuperBox::ParseBoxContent","run into an EOF while parsing a box header in a superbox");
    lbox = (hi << 16) | lo;
    //
    // Parse off tbox.
    hi   = stream->GetWord();
    lo   = stream->GetWord();
    if (lo == ByteStream::EOF)
      JPG_THROW(UNEXPECTED_EOF,"SuperBox::ParseBoxContent","run into an EOF while parsing a box header in a superbox");
    tbox = (hi << 16) | lo;
    //
    // Check whether we need an xlbox field. This happens if lbox is one.
    if (lbox == 1) {
      LONG l1,l2,l3,l4;
      if (boxsize < 4 + 4 + 8)
        JPG_THROW(MALFORMED_STREAM,"SuperBox::ParseBoxContent","found incomplete box header within a superbox");
      //
      // Read XLBox.
      l1 = stream->GetWord();
      l2 = stream->GetWord();
      l3 = stream->GetWord();
      l4 = stream->GetWord();
      if (l4 == ByteStream::EOF)
        JPG_THROW(UNEXPECTED_EOF,"SuperBox::ParseBoxContent","run into an EOF while parsing a box header in a superbox");
      //
      xlbox = (UQUAD(l1) << 48) | (UQUAD(l2) << 32) | (UQUAD(l3) << 16) | (UQUAD(l4) << 0);
      //
      // Check for consisteny. The xlbox size needs to include tbox,lbox and xlbox.
      if (xlbox < 4 + 4 + 8)
        JPG_THROW(MALFORMED_STREAM,"SuperBox::ParseBoxContent","box size within super box is inconsistent and too short");
      overhead = 4 + 4 + 8;
    } else if (lbox == 0) {
      // This is actually not part of the standard. It could mean (and it does mean, in J2K) that the
      // box extends to the end of the superbox, or the end of the EOF.
      JPG_THROW(MALFORMED_STREAM,"SuperBox::ParseBoxContent","found a box size of zero within a superbox");
    } else if (lbox < 4 + 4) {
      // The box is too short and does not include its own size fields.
      JPG_THROW(MALFORMED_STREAM,"SuperBox::ParseBoxContent","box size within super box is inconsistent and too short");
    } else {
      // This is a regularly sized box. Use xlbox for the box size from this point on.
      xlbox    = lbox;
      overhead = 4 + 4;
    }
    //
    // Check whether there are enough bytes left for the body of this box.
    if (boxsize < xlbox)
      JPG_THROW(MALFORMED_STREAM,"SuperBox::ParseBoxContent",
                "incomplete super box, super box does not provide enough data for body of sub-box");
    //
    // Create a new sub-box of this box.
    box      = CreateBox(tbox);
    if (box == NULL) {
      // Done parsing this box, remove the bytes.
      boxsize -= xlbox;
      // Nobody interested in this box. Just skip the body bytes.
      xlbox   -= overhead;
      while(xlbox > 0) {
        UWORD skip;
        if (xlbox > MAX_UWORD) {
          skip = MAX_UWORD;
        } else {
          skip = UWORD(xlbox);
        }
        stream->SkipBytes(skip);
        xlbox -= skip;
      }
      //
      //
    } else {
      // Note that the box size does not need to be set here. It is private to the
      // box and is already known. It is a temporary to the parser.
      //
      // Parse now the body of the box. It is complete by definition since the
      // superbox is complete.
      if (box->ParseBoxContent(stream,xlbox - overhead)) {
        // Done. Reduce the number of available bytes.
        boxsize -= xlbox;
        // Inform the superbox that the box is now created.
        AcknowledgeBox(box,tbox);
      } else if (xlbox - overhead <= MAX_ULONG) {
        // Push this into the decoder stream of the box and
        // let the box do its parsing when it feels like it.
        box->InputStreamOf()->Append(stream,xlbox - overhead,0);
        boxsize -= xlbox;
      } else {
        // Input is too long. I don't want to buffer >4GB.
        JPG_THROW(OVERFLOW_PARAMETER,"SuperBox::ParseBoxContent",
                  "sub-box of a superbox is too long (>4GB) for buffering");
      }
    }
    // Done with this box. Continue until all bytes done.
  }
  //
  // Done parsing. If there is a buffer, be gone with it.
  return true;
}
///

/// SuperBox::CreateBox
// Create a new box within the box list of this superbox, or return
// NULL in case the box type is irrelevant.
class Box *SuperBox::CreateBox(ULONG tbox)
{
  return CreateBox(m_pSubBoxes,tbox);
}
///

/// SuperBox::CreateBoxContent
// Write the super box content, namely all the boxes, into the output stream.
// This also calls the superbox implementation to perform the operation.
bool SuperBox::CreateBoxContent(class MemoryStream *target)
{
  class Box *box = m_pSubBoxes;
  bool done      = true;
  // Contents of the subboxes now go into the memory output stream
  // for buffering and determining their size. This is computed last.
  while(box) {
    // Create the content of the sub-box, buffer all its data.
    if (box->CreateBoxContent(box->OutputStreamOf())) {
      // Buffering done. Determine the size of the output stream.
      // This writes without an enumerator because we are in a superbox.
      box->WriteBoxContent(target);
    } else {
      // This is a sub-box which does not yet know its size. We cannot do that within
      // super boxes.
      done = false;
      assert(!"found subbox of a superbox that is not yet complete - this cannot work");
    }
    //
    // Advance to the next box.
    box = box->NextOf();
  }
  //
  return done;
}
///

  
