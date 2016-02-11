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
