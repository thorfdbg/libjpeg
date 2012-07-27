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
** This class is responsible for parsing the huffman specific part of the
** DHT marker and generating the corresponding decoder classes.
**
** $Id: huffmantemplate.cpp,v 1.12 2012-06-02 10:27:13 thor Exp $
**
*/

/// Includes
#include "coding/huffmantemplate.hpp"
#include "io/bytestream.hpp"
#include "coding/huffmancoder.hpp"
#include "coding/huffmandecoder.hpp"
#include "coding/huffmanstatistics.hpp"
///

/// HuffmanTemplate::HuffmanTemplate
HuffmanTemplate::HuffmanTemplate(class Environ *env)
  : JKeeper(env), m_pucValues(NULL), m_pEncoder(NULL), m_pDecoder(NULL), m_pStatistics(NULL)
{
}
///

/// HuffmanTemplate::~HuffmanTemplate
HuffmanTemplate::~HuffmanTemplate(void)
{
  if (m_pucValues)
    m_pEnviron->FreeMem(m_pucValues,sizeof(UBYTE) * m_ulCodewords);
  
  delete m_pDecoder;
  delete m_pEncoder;
  delete m_pStatistics;
}
///

/// HuffmanTemplate::WriteMarker
// Write the huffman table stored here.
void HuffmanTemplate::WriteMarker(class ByteStream *io)
{
  ULONG i;
  ULONG total = 0;
  //
  // Write the number of huffman codes of lengths i-1
  for(i = 0;i < 16U;i++) {
    io->Put(m_ucLengths[i]);
    total += m_ucLengths[i];
  }
  //
  // Write the symbols.
  for(i = 0;i < total;i++) {
    io->Put(m_pucValues[i]);
  }
}
///

/// HuffmanTemplate::MarkerOverhead
// Return the space required to write this part of the marker
UWORD HuffmanTemplate::MarkerOverhead(void) const
{
  ULONG size  = 0;
  ULONG i;
  //
  // Write the number of huffman codes of lengths i-1
  for(i = 0;i < 16U;i++) {
    size++;
    size += m_ucLengths[i];
  }

  if (size > MAX_UWORD)
    JPG_THROW(OVERFLOW_PARAMETER,"HuffmanTemplate::MarkerOverhead","DHT huffman table too long");

  return size;
}
///

/// HuffmanTemplate::ResetEntries
// Reset the huffman table for an alphabet with N entries.
void HuffmanTemplate::ResetEntries(ULONG count)
{ 
  if (m_pucValues) {
    m_pEnviron->FreeMem(m_pucValues,sizeof(UBYTE) * m_ulCodewords);
    m_pucValues = NULL;
  }

  delete m_pDecoder;m_pDecoder = NULL;
  delete m_pEncoder;m_pEncoder = NULL;
  // The statistics remains valid.

  m_ulCodewords = count;
  if (count > 0)
    m_pucValues = (UBYTE *)m_pEnviron->AllocMem(sizeof(UBYTE) * m_ulCodewords);

  memset(m_ucLengths,0,sizeof(m_ucLengths));
}
///

/// HuffmanTemplate::InitDCLuminanceDefault
// Install the default Luminance DC table.
void HuffmanTemplate::InitDCLuminanceDefault(void)
{
  static const UBYTE bits_dc_luminance[] = { 0, 1, 5, 1, 1, 1, 1, 1, 1 };
  static const UBYTE val_dc_luminance[]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  ResetEntries(sizeof(val_dc_luminance));

  memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
  memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
}
///

/// HuffmanTemplate::InitDCChrominanceDefault
// Install the default Chrominance DC table.
void HuffmanTemplate::InitDCChrominanceDefault(void)
{
  static const UBYTE bits_dc_chrominance[] = { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  static const UBYTE val_dc_chrominance[]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  ResetEntries(sizeof(val_dc_chrominance));
  
  memcpy(m_ucLengths,bits_dc_chrominance,sizeof(bits_dc_chrominance));
  memcpy(m_pucValues,val_dc_chrominance ,sizeof(val_dc_chrominance));
}
///

/// HuffmanTemplate::InitACLuminanceDefault
// Install the default luminance AC table
void HuffmanTemplate::InitACLuminanceDefault(void)
{ 
  static const UBYTE bits_ac_luminance[] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UBYTE val_ac_luminance[]  = { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
					     0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
					     0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
					     0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
					     0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
					     0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
					     0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
					     0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
					     0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
					     0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
					     0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
					     0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
					     0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
					     0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
					     0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
					     0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
					     0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
					     0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
					     0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
					     0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
					     0xf9, 0xfa };

  ResetEntries(sizeof(val_ac_luminance));

  memcpy(m_ucLengths,bits_ac_luminance,sizeof(bits_ac_luminance));
  memcpy(m_pucValues,val_ac_luminance ,sizeof(val_ac_luminance));
}
///

/// HuffmanTemplate::InitACChrominanceDefault
// Install the default chrominance AC table
void HuffmanTemplate::InitACChrominanceDefault(void)
{  
  static const UBYTE bits_ac_chrominance[] = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UBYTE val_ac_chrominance[] =  { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
					       0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
					       0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
					       0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
					       0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
					       0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
					       0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
					       0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
					       0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
					       0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
					       0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
					       0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
					       0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
					       0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
					       0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
					       0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
					       0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
					       0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
					       0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
					       0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
					       0xf9, 0xfa };

  ResetEntries(sizeof(val_ac_chrominance));
  
  memcpy(m_ucLengths,bits_ac_chrominance,sizeof(bits_ac_chrominance));
  memcpy(m_pucValues,val_ac_chrominance ,sizeof(val_ac_chrominance));
}
///

/// HuffmanTemplate::BuildEncoder
// Build the huffman encoder given the template data.
void HuffmanTemplate::BuildEncoder(void)
{
  assert(m_pEncoder == NULL);
  
  //
  // If the coder is not used, do not build it.
  if (m_pucValues) {
    assert(m_ucLengths);
    m_pEncoder = new(m_pEnviron) class HuffmanCoder(m_ucLengths,m_pucValues);
  }
}
///

/// HuffmanTemplate::BuildStatistics
// Build the huffman statistics.
void HuffmanTemplate::BuildStatistics(void)
{
  assert(m_pStatistics == NULL);

  m_pStatistics = new(m_pEnviron) class HuffmanStatistics();
}
///

/// HuffmanTemplate::BuildDecoder
// Build the huffman decoder given the template data.
void HuffmanTemplate::BuildDecoder(void)
{ 
  class HuffmanDecoder *prev = NULL;
  ULONG i,lastbits = 0;
  UBYTE *values;

  assert(m_pDecoder == NULL);

  if (m_pucValues) {
    // If the decoder is not used, do not build it.
    assert(m_ucLengths);
    for(i = 0,values = m_pucValues;i < 16U;i++) {
      if (m_ucLengths[i]) {
	if (values + m_ucLengths[i] > m_pucValues + m_ulCodewords)
	  JPG_THROW(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker","huffman table marker depends on undefined data");
	// There are codes of this lengths.
	class HuffmanDecoder *huff = new(m_pEnviron) class HuffmanDecoder(values,i+1-lastbits,
									  m_ucLengths[i]);
	if (prev) {
	  prev->ExtendBy(huff);
	} else {
	  assert(m_pDecoder == NULL);
	  m_pDecoder = huff;
	}
	prev     = huff;
	values  += m_ucLengths[i];
	lastbits = i+1;
      }
    }
  }
}
///

/// HuffmanTemplate::ParseMarker
void HuffmanTemplate::ParseMarker(class ByteStream *io)
{
  ULONG i,total = 0;
  // A new decoder chain is required here.
  delete m_pDecoder;m_pDecoder = NULL;
  delete m_pEncoder;m_pEncoder = NULL;
  
  // Read the number of huffman codes of length i-1
  for(i = 0;i < 16U;i++) {
    LONG cnt = io->Get();
    if (cnt == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker","huffman table marker run out of data");
    m_ucLengths[i] = cnt;
    total         += cnt;
  }
  
  m_ulCodewords = total;
  assert(m_pucValues == NULL);
  m_pucValues = (UBYTE *)m_pEnviron->AllocMem(sizeof(UBYTE) * total);

  for(i = 0;i < total;i++) {
    LONG v = io->Get();
    if (v == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker","huffman table marker run out of data");
    m_pucValues[i] = v;
  }
}
///

/// HuffmanTemplate::AdjustToStatistics
// Use the collected statistics to build an optimized
// huffman table.
void HuffmanTemplate::AdjustToStatistics(void)
{
  if (m_pStatistics) {
    const UBYTE *codesizes = m_pStatistics->CodesizesOf();
    UBYTE *v;
    ULONG total = 0;
    int i,j;
    //
    ResetEntries(0);
    //
    // Update the histogram of how many code sizes appear how often.
    for(i = 0;i < 256;i++) {
      if (codesizes[i] > 0) {
	m_ucLengths[codesizes[i]-1]++; // is one-based.
	total++;
      }
    }
    //
    // Now update the codeword table. 
    assert(m_pucValues == NULL);
    m_ulCodewords = total;
    m_pucValues   = (UBYTE *)m_pEnviron->AllocMem(sizeof(UBYTE) * m_ulCodewords);
    //
    // Sort codevalues in. i enumerates the
    // code size in increasing order, j
    // enumerates the codes.
    for(i = 1,v = m_pucValues;i <= 16;i++) {
      for(j = 0;j < 256;j++) {
	if (codesizes[j] == i) {
	  *v++ = j; // assign symbol j the next code
	}
      }
    }
    
    delete m_pStatistics;
    m_pStatistics = NULL;
  }
}
///
