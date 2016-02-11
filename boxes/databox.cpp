/*
** This box implements a data container for refinement scans, as hidden
** in the APP11 markers.
**
** $Id: databox.cpp,v 1.10 2014/09/30 08:33:14 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "boxes/databox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
#include "io/decoderstream.hpp"
///

/// DataBox::EncoderBufferOf
// Return the byte stream buffer where the encoder can drop the data
// on encoding.
ByteStream *DataBox::EncoderBufferOf(void)
{
  return OutputStreamOf();
}
///

/// DataBox::DecoderBufferOf
// Return the stream the decoder will decode from.
ByteStream *DataBox::DecoderBufferOf(void)
{
  return InputStreamOf();
}
///

/// DataBox::Flush
// Flush the buffered data of the box and create the markers.
void DataBox::Flush(class ByteStream *target,UWORD enumerator)
{
  WriteBoxContent(target,enumerator);
}
///
