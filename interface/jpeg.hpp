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
** Definition of the library interface
**
** This file defines the main entry points and user accessible data
** for the 10918 (JPEG) codec. Except for the tagitem and hook methods,
** no other headers should be publically accessible.
** 
** $Id: jpeg.hpp,v 1.4 2012-06-02 10:27:14 thor Exp $
**
*/

#ifndef JPEG_HPP
#define JPEG_HPP

/// Includes
///

/// Forwards
class Environment;
class Encoder;
class Decoder;
class IOStream;
class Image;
class Frame;
class Scan;
///

/// Class JPEG
// This is the main entry class for the JPEG encoder and decoder.
// It is basically a pimpl for the actual codec.
class JPEG {
  //
  // Constructors and destructors are not publically accessible, use
  // the construction and destruction calls below. 
  JPEG(void);
  JPEG(const JPEG &);
  const JPEG &operator =(const JPEG &);
  ~JPEG(void);
  //
  //
  // The environment, i.e. the class that is responsible for the memory
  // management.
  class Environ  *m_pEnviron;
  // 
  // The encoder
  class Encoder  *m_pEncoder;
  //
  // The decoder
  class Decoder  *m_pDecoder; 
  //
  // Currently active IOHook to read and write data to the filing system.
  class IOStream *m_pIOStream;
  //
  // Currently loaded image, if any.
  class Image  *m_pImage;
  //
  // Currently active frame, to be read or to be written.
  class Frame  *m_pFrame;
  //
  // Currently active scan, to be read or to be written.
  class Scan   *m_pScan;
  //
  // Currently in parsing an MCU row?
  bool          m_bRow;
  //
  // Currently decoding active?
  bool          m_bDecoding;
  //
  // Currently encoding active?
  bool          m_bEncoding;
  //
  // Image header written?
  bool          m_bHeaderWritten;
  //
  // Huffman optimization step done?
  bool          m_bOptimized;
  //
  // Requires optimization?
  bool          m_bOptimizeHuffman;
  //
  // The real constructor. We must use this, since we're not using
  // NEW to allocate objects, but MALLOC.
  void doConstruct(class Environ *env);
  //
  // The real destructor. We must use this, since we're not using
  // NEW to allocate objects, but MALLOC.
  void doDestruct(void);
  //
  // Read a file. Exceptions are thrown here and captured outside.
  void ReadInternal(struct JPG_TagItem *tags);
  //
  // Write a file. Exceptions are thrown here and captured outside.
  void WriteInternal(struct JPG_TagItem *tags);
  //
  // Reverse transform a given rectangle, internal version that
  // throws exceptions.
  void InternalDisplayRectangle(struct JPG_TagItem *tags);
  //
  // Forward transform an image, push it into the encoder - this is the internal
  // version that generates exceptions.
  void InternalProvideImage(struct JPG_TagItem *tags);
  //
  // Request information from the JPEG object - the internal version that creates exceptions.
  void InternalGetInformation(struct JPG_TagItem *tags);
  //
public:
  //
  // Create an instance of this class.
  static class JPEG *Construct(struct JPG_TagItem *);
  //
  // Destroy a previously created instance.
  static void Destruct(class JPEG *);
  //
  // Read a file. This takes all of the tags, class Decode takes.
  JPG_LONG Read(struct JPG_TagItem *);
  //
  // Write a file. This takes all of the tags, class Encode takes.
  JPG_LONG Write(struct JPG_TagItem *);
  //
  // Reverse transform a given rectangle
  JPG_LONG DisplayRectangle(struct JPG_TagItem *);
  //
  // Forward transform an image, push it into the encoder.
  JPG_LONG ProvideImage(struct JPG_TagItem *);
  //
  // Request information from the JPEG object.
  JPG_LONG GetInformation(struct JPG_TagItem *);
  //
  // Return the last exception - the error code, if present - in
  // the primary result code, a pointer to the error string in the
  // argument. If no error happened, return 0. For finer error handling,
  // define an exception hook upon creation of the library.
  JPG_LONG LastError(const char *&error);
  //
  // Return the last warning - the error code, if present - in the
  // primary result code and the warning message in the argument. Returns
  // 0 if no warning happened. Define a warning hook for finer warning
  // control.
  JPG_LONG LastWarning(const char *&warning);
};
///

///
#endif
