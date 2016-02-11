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
