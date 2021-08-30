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
** Definition of the library interface
**
** This file defines the main entry points and user accessible data
** for the 10918 (JPEG) codec. Except for the tagitem and hook methods,
** no other headers should be publically accessible.
** 
** $Id: jpeg.hpp,v 1.16 2021/01/29 15:34:39 thor Exp $
**
*/

#ifndef JPEG_HPP
#define JPEG_HPP

/// Includes
#include "interface/jpgtypes.hpp"
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

/// Defines
#ifndef JPG_EXPORT
#define JPG_EXPORT
#endif
///

/// Class JPEG
// This is the main entry class for the JPEG encoder and decoder.
// It is basically a pimpl for the actual codec.
class JPG_EXPORT JPEG {
  friend struct JPEG_Helper;
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
  // Requires R/D optimization with a langrangian multiplier?
  bool          m_bOptimizeQuantizer;
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
  // Stop decoding, then return. Also tests the checksum if there is one.
  void StopDecoding(void);
  //
  // Check whether any of the scans is optimized Huffman and thus requires a two-pass
  // go over the data.
  bool RequiresTwoPassEncoding(const struct JPG_TagItem *tags) const;
  //
  // Return layout information about floating point and conversion from the specs
  // and insert it into the given tag list.
  void GetOutputInformation(class MergingSpecBox *specs,struct JPG_TagItem *tags) const;
  //
  // Return the marker at the current stream position or -1 for an error.
  // tags is currently unused.
  JPG_LONG InternalPeekMarker(struct JPG_TagItem *) const;
  //
  // Read data from the current stream position, return -1 for an error.
  JPG_LONG InternalReadMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *tags) const;
  //
  // Skip over the given number of bytes.
  JPG_LONG InternalSkipMarker(JPG_LONG bytes,struct JPG_TagItem *) const;
  //
  // Write data to the current stream position.
  JPG_LONG InternalWriteMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *tags) const;
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
  // In case reading was interrupted by a JPGTAG_DECODER_STOP mask
  // at some point in the codestream, this call returns the next
  // 16 bits at the current stop position without removing them
  // from the stream. You may want to use these bits to decide
  // whether you (instead of the library) want to parse the following
  // data off yourself with the calls below. This call returns -1 on
  // an EOF or any other error, and it returns 0 in case a marker has
  // been detected the core MUST handle itself and an external
  // code CANNOT possibly parse off.
  // Currently, the tags argument is not used, just pass NULL.
  JPG_LONG PeekMarker(struct JPG_TagItem *);
  //
  // In case reading was interrupted by a JPGTAG_DECODER_STOP mask,
  // remove parts of the data from the stream outside of the library.
  // This call reads the given number of bytes into the supplied
  // buffer and returns the number of bytes it was able to read, or -1
  // for an error. The tags argument is currently unused and should be
  // set to NULL.
  JPG_LONG ReadMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *);
  //
  // Skip over the given number of bytes. Returns -1 for failure, anything
  // else for success. The tags argument is currently unused and should
  // be set to NULL.
  JPG_LONG SkipMarker(JPG_LONG bytes,struct JPG_TagItem *);
  //
  // In case writing was interrupted by a JPGTAG_ENCODER_STOP mask,
  // this call can be used to inject additional data into the codestream.
  // The typical application of this call is to inject custom markers.
  // It writes the bytes in the buffer of the given size to the stream,
  // and returns the number of bytes it could write, or -1 for an error.
  // The tags argument is currently unused and should be set to NULL.
  JPG_LONG WriteMarker(void *buffer,JPG_LONG bufsize,struct JPG_TagItem *);
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
