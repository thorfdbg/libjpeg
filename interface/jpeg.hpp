
/*
** Definition of the library interface
**
** This file defines the main entry points and user accessible data
** for the 10918 (JPEG) codec. Except for the tagitem and hook methods,
** no other headers should be publically accessible.
** 
** $Id: jpeg.hpp,v 1.13 2015/03/11 16:02:42 thor Exp $
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

/// Class JPEG
// This is the main entry class for the JPEG encoder and decoder.
// It is basically a pimpl for the actual codec.
class JPEG {
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
