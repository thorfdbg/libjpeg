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
** This class is responsible for parsing the huffman specific part of the
** DHT marker and generating the corresponding decoder classes.
**
** $Id: huffmantemplate.cpp,v 1.41 2024/06/25 06:31:41 thor Exp $
**
*/

/// Includes
#include "coding/huffmantemplate.hpp"
#include "io/bytestream.hpp"
#include "coding/huffmancoder.hpp"
#include "coding/huffmandecoder.hpp"
#include "coding/huffmanstatistics.hpp"
#ifdef COLLECT_STATISTICS
#include "std/stdio.hpp"
#endif
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
void HuffmanTemplate::InitDCLuminanceDefault(ScanType type,UBYTE depth,UBYTE,UBYTE scanidx)
{

#ifdef COLLECT_STATISTICS
  m_bAC     = false;
  m_bChroma = false;
  m_ucScan  = scanidx;
#else
  NOREF(scanidx);
#endif

  switch(type) {
  case Baseline:
  case Sequential:
    if (depth == 8) { 
      static const UBYTE bits_dc_luminance[] = { 0, 1, 5, 1, 1, 1, 1, 1, 1 };
      static const UBYTE val_dc_luminance[]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

      ResetEntries(sizeof(val_dc_luminance));
      
      memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
      memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_dc_luminance[] = {0, 0, 6, 2, 3, 1, 1, 1, 1, 1 };
      static const UBYTE val_dc_luminance[]  = {5, 6, 7, 8, 9, 10,
                                                4, 11,
                                                2, 3, 12,
                                                1, 0, 13, 14, 15};
      ResetEntries(sizeof(val_dc_luminance));
      
      memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
      memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
      return;
    }
    break;  
  case Progressive:
    if (depth == 8) { 
      static const UBYTE bits_dc_luminance[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
      static const UBYTE val_dc_luminance[]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

      ResetEntries(sizeof(val_dc_luminance));

      memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
      memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_dc_luminance[] = {0, 0, 6, 2, 3, 1, 1, 1, 1, 1 };
      static const UBYTE val_dc_luminance[]  = {5, 6, 7, 8, 9, 10,
                                                4, 11,
                                                2, 3, 12,
                                                1, 0, 13, 14, 15};
      ResetEntries(sizeof(val_dc_luminance));
      
      memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
      memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
      return;
    }
    break; 
  case Lossless:
    { 
      static const UBYTE bits_dc_luminance[] = { 0,0,4,6,2,3,1,1 };
      static const UBYTE val_dc_luminance[]  = { 0, 1, 2, 7,
                                                 3, 4, 5, 6, 8, 9,
                                                 10, 15,
                                                 11, 13, 14,
                                                 12,
                                                 16 };
      ResetEntries(sizeof(val_dc_luminance));
      
      memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
      memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
      return;
    }
    break; 
  case ACSequential:
  case ACProgressive:
  case ACLossless:
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
  case ACDifferentialLossless:
  case ACResidual:
  case ACResidualProgressive:
  case ACResidualDCT:
  case JPEG_LS:
    assert(!"internal coding error - no Huffman table should be required for the selected coding mode");
    break;
  default:
    break;
  } 

  ResetEntries(1);
}
///

/// HuffmanTemplate::InitDCChrominanceDefault
// Install the default Chrominance DC table.
void HuffmanTemplate::InitDCChrominanceDefault(ScanType type,UBYTE depth,UBYTE,UBYTE scanidx)
{

#ifdef COLLECT_STATISTICS
  m_bAC     = false;
  m_bChroma = true;
  m_ucScan  = scanidx;
#else
  NOREF(scanidx);
#endif

  switch(type) {
  case Baseline:
  case Sequential:
    if (depth == 8) {
      static const UBYTE bits_dc_chrominance[] = { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1};
      static const UBYTE val_dc_chrominance[]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
      
      ResetEntries(sizeof(val_dc_chrominance));
      
      memcpy(m_ucLengths,bits_dc_chrominance,sizeof(bits_dc_chrominance));
      memcpy(m_pucValues,val_dc_chrominance ,sizeof(val_dc_chrominance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_dc_chrominance[] = { 0, 1, 4, 2, 3, 1, 1, 1, 1, 1, 1 };
      static const UBYTE val_dc_chrominance[]  = { 5,
                                                   3, 4, 6, 7,
                                                   2, 8,
                                                   1, 9, 10,
                                                   0, 11, 12, 13, 14, 15 };
      
      ResetEntries(sizeof(val_dc_chrominance));
      
      memcpy(m_ucLengths,bits_dc_chrominance,sizeof(bits_dc_chrominance));
      memcpy(m_pucValues,val_dc_chrominance ,sizeof(val_dc_chrominance));
      return;
    }
    break; 
  case Progressive:
    if (depth == 8) { 
      static const UBYTE bits_dc_luminance[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
      static const UBYTE val_dc_luminance[]  = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
      
      ResetEntries(sizeof(val_dc_luminance));

      memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
      memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_dc_chrominance[] = { 0, 1, 4, 2, 3, 1, 1, 1, 1, 1, 1 };
      static const UBYTE val_dc_chrominance[]  = { 5,
                                                   3, 4, 6, 7,
                                                   2, 8,
                                                   1, 9, 10,
                                                   0, 11, 12, 13, 14, 15 };
      
      ResetEntries(sizeof(val_dc_chrominance));
      
      memcpy(m_ucLengths,bits_dc_chrominance,sizeof(bits_dc_chrominance));
      memcpy(m_pucValues,val_dc_chrominance ,sizeof(val_dc_chrominance));
      return;
    }
    break; 
  case Lossless:
    if (depth == 8) { 
      static const UBYTE bits_dc_luminance[] = { 0,0,1,4,3,2,3,1,241,1};
      static const UBYTE val_dc_luminance[]  = { 0,
                                                 1, 2, 5, 6,
                                                 3, 4, 7,
                                                 8, 15,
                                                 9, 13, 14,
                                                 12,
                                                 11, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 
                                                 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
                                                 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 
                                                 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 
                                                 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 
                                                 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 
                                                 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 
                                                 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 
                                                 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 
                                                 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 
                                                 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 
                                                 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 
                                                 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 
                                                 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 
                                                 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 
                                                 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 
                                                 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 
                                                 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 
                                                 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 
                                                 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 
                                                 253, 254, 255, 
                                                 10 };

      ResetEntries(sizeof(val_dc_luminance));
      
      memcpy(m_ucLengths,bits_dc_luminance,sizeof(bits_dc_luminance));
      memcpy(m_pucValues,val_dc_luminance ,sizeof(val_dc_luminance));
      return;
    }
    break; 
  case ACSequential:
  case ACProgressive:
  case ACLossless:
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
  case ACDifferentialLossless:
  case ACResidual:
  case ACResidualProgressive:
  case ACResidualDCT:
  case JPEG_LS:
    assert(!"internal coding error - no Huffman table should be required for the selected coding mode");
    break;
  default:
    break;
  }  
  
  ResetEntries(1);
}
///

/// HuffmanTemplate::InitACLuminanceDefault
// Install the default luminance AC table
void HuffmanTemplate::InitACLuminanceDefault(ScanType type,UBYTE depth,UBYTE,UBYTE scanidx)
{ 
 

#ifdef COLLECT_STATISTICS
  m_bAC     = true;
  m_bChroma = false;
  m_ucScan  = scanidx;
#else
  NOREF(scanidx);
#endif

  switch(type) {
  case Baseline:
  case Sequential:
  case DifferentialSequential:
    if (depth == 8) { 
      static const UBYTE bits_ac_luminance[] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
      static const UBYTE val_ac_luminance[]  = { 0x01, 0x02, 
                                                 0x03, 
                                                 0x00, 0x04, 0x11, 
                                                 0x05, 0x12, 0x21, 
                                                 0x31, 0x41, 
                                                 0x06, 0x13, 0x51, 0x61, 
                                                 0x07, 0x22, 0x71, 
                                                 0x14, 0x32, 0x81, 0x91, 0xa1, 
                                                 0x08, 0x23, 0x42, 0xb1, 0xc1, 
                                                 0x15, 0x52, 0xd1, 0xf0, 
                                                 0x24, 0x33, 0x62, 0x72, 
                                                 0x82, 
                                                 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 
                                                 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 
                                                 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 
                                                 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 
                                                 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 
                                                 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 
                                                 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 
                                                 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 
                                                 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 
                                                 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 
                                                 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 
                                                 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 
                                                 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 
                                                 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 
                                                 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 
                                                 0xf6, 0xf7, 0xf8, 0xf9, 0xfa };
      ResetEntries(sizeof(val_ac_luminance));
      
      memcpy(m_ucLengths,bits_ac_luminance,sizeof(bits_ac_luminance));
      memcpy(m_pucValues,val_ac_luminance ,sizeof(val_ac_luminance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_ac_luminance[] = {0, 1, 3, 3, 2, 4, 4, 2, 5, 3, 4, 6, 5, 6,
                                                207, 1};
      static const UBYTE val_ac_luminance[]  = { 1,
                                                 2, 3, 4,
                                                 5, 6, 17,
                                                 7, 18,
                                                 0, 8, 9, 33,
                                                 10, 19, 34, 49,
                                                 20, 65,
                                                 11, 21, 50, 81, 97,
                                                 35, 113, 129,
                                                 12, 22, 66, 145,
                                                 23, 36, 51, 82, 161, 177,
                                                 13, 98, 193, 209, 240,
                                                 24, 37, 67, 114, 225, 241,
                                                 14, 15, 16, 25, 26, 27, 28, 29, 30, 31,
                                                 32, 38, 39, 40, 41, 42, 43, 44, 45, 46,
                                                 47, 48, 52, 53, 54, 55, 56, 57, 58, 59,
                                                 60, 61, 62, 63, 64, 68, 69, 70, 71, 72,
                                                 73, 74, 75, 76, 77, 78, 79, 80, 83, 84,
                                                 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
                                                 95, 96, 99, 100, 101, 102, 103, 104, 105,
                                                 106, 107, 108, 109, 110, 111, 112, 115,
                                                 116, 117, 118, 119, 120, 121, 122, 123,
                                                 124, 125, 126, 127, 128, 130, 131, 132,
                                                 133, 134, 135, 136, 137, 138, 139, 140,
                                                 141, 142, 143, 144, 146, 147, 148, 149,
                                                 150, 151, 152, 153, 154, 155, 156, 157,
                                                 158, 159, 160, 162, 163, 164, 165, 166,
                                                 167, 168, 169, 170, 171, 172, 173, 174,
                                                 175, 176, 178, 179, 180, 181, 182, 183,
                                                 184, 185, 186, 187, 188, 189, 190, 191,
                                                 192, 194, 195, 196, 197, 198, 199, 200,
                                                 201, 202, 203, 204, 205, 206, 207, 208,
                                                 210, 211, 212, 213, 214, 215, 216, 217,
                                                 218, 219, 220, 221, 222, 223, 224, 226,
                                                 227, 228, 229, 230, 231, 232, 233, 234,
                                                 235, 236, 237, 238, 239, 242, 243, 244,
                                                 245, 246, 247, 248, 249, 250, 251, 252,
                                                 253, 254,
                                                 255};
      ResetEntries(sizeof(val_ac_luminance));
      
      memcpy(m_ucLengths,bits_ac_luminance,sizeof(bits_ac_luminance));
      memcpy(m_pucValues,val_ac_luminance ,sizeof(val_ac_luminance));
      return;
    }
    break;
  case Progressive:  
    if (depth == 8) {
      static const UBYTE bits_ac_chrominance[] = { 0,3,0,1,2,4,4,3,4,5,4,4,3,2,4,133};
      static const UBYTE val_ac_chrominance[]  = { 0, 1, 17,
                                                   33,
                                                   16, 49,
                                                   2, 32, 65, 81,
                                                   3, 18, 48, 97,
                                                   64, 113, 129,
                                                   34, 80, 145, 161,
                                                   4, 19, 50, 96, 177,
                                                   112, 193, 209, 240,
                                                   20, 51, 66, 225,
                                                   35, 128, 241,
                                                   82, 114,
                                                   5, 52, 98, 144,
                                                   6, 7, 8, 9, 10, 21, 22, 23,
                                                   24, 25, 26, 36, 37, 38, 39, 40,
                                                   41, 42, 53, 54, 55, 56, 57, 58,
                                                   67, 68, 69, 70, 71, 72, 73, 74,
                                                   83, 84, 85, 86, 87, 88, 89, 90,
                                                   99, 100, 101, 102, 103, 104, 105, 106,
                                                   115, 116, 117, 118, 119, 120, 121, 122,
                                                   130, 131, 132, 133, 134, 135, 136, 137,
                                                   138, 146, 147, 148, 149, 150, 151, 152,
                                                   153, 154, 160, 162, 163, 164, 165, 166,
                                                   167, 168, 169, 170, 176, 178, 179, 180,
                                                   181, 182, 183, 184, 185, 186, 192, 194,
                                                   195, 196, 197, 198, 199, 200, 201, 202,
                                                   208, 210, 211, 212, 213, 214, 215, 216,
                                                   217, 218, 224, 226, 227, 228, 229, 230,
                                                   231, 232, 233, 234, 242, 243, 244, 245,
                                                   246, 247, 248, 249, 250};
      
      ResetEntries(sizeof(val_ac_chrominance));

      memcpy(m_ucLengths,bits_ac_chrominance,sizeof(bits_ac_chrominance));
      memcpy(m_pucValues,val_ac_chrominance ,sizeof(val_ac_chrominance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_ac_luminance[] = {0, 1, 3, 3, 2, 4, 4, 2, 5, 3, 4, 6, 5, 6,
                                                207, 1};
      static const UBYTE val_ac_luminance[]  = { 1,
                                                 2, 3, 4,
                                                 5, 6, 17,
                                                 7, 18,
                                                 0, 8, 9, 33,
                                                 10, 19, 34, 49,
                                                 20, 65,
                                                 11, 21, 50, 81, 97,
                                                 35, 113, 129,
                                                 12, 22, 66, 145,
                                                 23, 36, 51, 82, 161, 177,
                                                 13, 98, 193, 209, 240,
                                                 24, 37, 67, 114, 225, 241,
                                                 14, 15, 16, 25, 26, 27, 28, 29, 30, 31,
                                                 32, 38, 39, 40, 41, 42, 43, 44, 45, 46,
                                                 47, 48, 52, 53, 54, 55, 56, 57, 58, 59,
                                                 60, 61, 62, 63, 64, 68, 69, 70, 71, 72,
                                                 73, 74, 75, 76, 77, 78, 79, 80, 83, 84,
                                                 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
                                                 95, 96, 99, 100, 101, 102, 103, 104, 105,
                                                 106, 107, 108, 109, 110, 111, 112, 115,
                                                 116, 117, 118, 119, 120, 121, 122, 123,
                                                 124, 125, 126, 127, 128, 130, 131, 132,
                                                 133, 134, 135, 136, 137, 138, 139, 140,
                                                 141, 142, 143, 144, 146, 147, 148, 149,
                                                 150, 151, 152, 153, 154, 155, 156, 157,
                                                 158, 159, 160, 162, 163, 164, 165, 166,
                                                 167, 168, 169, 170, 171, 172, 173, 174,
                                                 175, 176, 178, 179, 180, 181, 182, 183,
                                                 184, 185, 186, 187, 188, 189, 190, 191,
                                                 192, 194, 195, 196, 197, 198, 199, 200,
                                                 201, 202, 203, 204, 205, 206, 207, 208,
                                                 210, 211, 212, 213, 214, 215, 216, 217,
                                                 218, 219, 220, 221, 222, 223, 224, 226,
                                                 227, 228, 229, 230, 231, 232, 233, 234,
                                                 235, 236, 237, 238, 239, 242, 243, 244,
                                                 245, 246, 247, 248, 249, 250, 251, 252,
                                                 253, 254,
                                                 255};
      ResetEntries(sizeof(val_ac_luminance));
      
      memcpy(m_ucLengths,bits_ac_luminance,sizeof(bits_ac_luminance));
      memcpy(m_pucValues,val_ac_luminance ,sizeof(val_ac_luminance));
      return;
    }
    break;
  case ACSequential:
  case ACProgressive:
  case Lossless:
  case ACLossless:
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
  case ACDifferentialLossless:
  case ACResidual:
  case ACResidualDCT:
  case ACResidualProgressive:
  case JPEG_LS:
    assert(!"internal coding error - no Huffman table should be required for the selected coding mode");
    break;
  default:
    break;
  } 

  ResetEntries(1);
}
///

/// HuffmanTemplate::InitACChrominanceDefault
// Install the default chrominance AC table
void HuffmanTemplate::InitACChrominanceDefault(ScanType type,UBYTE depth,UBYTE,UBYTE scanidx)
{  
#ifdef COLLECT_STATISTICS
  m_bAC     = true;
  m_bChroma = true;
  m_ucScan  = scanidx;
#else
  NOREF(scanidx);
#endif

  switch(type) {
  case Baseline:
  case Sequential:
  case DifferentialSequential:
    if (depth == 8) { 
      static const UBYTE bits_ac_chrominance[] = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
      static const UBYTE val_ac_chrominance[] =  { 0x00, 0x01, 
                                                   0x02, 
                                                   0x03, 0x11, 
                                                   0x04, 0x05, 0x21, 0x31, 
                                                   0x06, 0x12, 0x41, 0x51, 
                                                   0x07, 0x61, 0x71,
                                                   0x13, 0x22, 0x32, 0x81, 
                                                   0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 
                                                   0x09, 0x23, 0x33, 0x52, 0xf0,
                                                   0x15, 0x62, 0x72, 0xd1, 
                                                   0x0a, 0x16, 0x24, 0x34,
                                                   0xe1, 
                                                   0x25, 0xf1, 
                                                   0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 
                                                   0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 
                                                   0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 
                                                   0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 
                                                   0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 
                                                   0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 
                                                   0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 
                                                   0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 
                                                   0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 
                                                   0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 
                                                   0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 
                                                   0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 
                                                   0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 
                                                   0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 
                                                   0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa };
      ResetEntries(sizeof(val_ac_chrominance));
      
      memcpy(m_ucLengths,bits_ac_chrominance,sizeof(bits_ac_chrominance));
      memcpy(m_pucValues,val_ac_chrominance ,sizeof(val_ac_chrominance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_ac_chrominance[] = { 0, 1, 3, 2, 4, 4, 4, 2, 6, 5, 4, 4,
                                                   3, 3, 8, 203};
      static const UBYTE val_ac_chrominance[] =  { 1,
                                                   2, 3, 4,
                                                   5, 17,
                                                   0, 6, 18, 33,
                                                   7, 19, 49, 65,
                                                   8, 34, 81, 97,
                                                   20, 113,
                                                   9, 35, 50, 129, 145, 161,
                                                   21, 66, 177, 193, 240,
                                                   10, 22, 209, 225,
                                                   36, 51, 82, 241,
                                                   11, 23, 98,
                                                   37, 67, 114,
                                                   12, 13, 14, 15, 24, 52, 130, 146,
                                                   16, 25, 26, 27, 28, 29, 30, 31, 32,
                                                   38, 39, 40, 41, 42, 43, 44, 45, 46,
                                                   47, 48, 53, 54, 55, 56, 57, 58, 59,
                                                   60, 61, 62, 63, 64, 68, 69, 70, 71,
                                                   72, 73, 74, 75, 76, 77, 78, 79, 80,
                                                   83, 84, 85, 86, 87, 88, 89, 90, 91,
                                                   92, 93, 94, 95, 96, 99, 100, 101, 102,
                                                   103, 104, 105, 106, 107, 108, 109, 110,
                                                   111, 112, 115, 116, 117, 118, 119, 120,
                                                   121, 122, 123, 124, 125, 126, 127, 128,
                                                   131, 132, 133, 134, 135, 136, 137, 138,
                                                   139, 140, 141, 142, 143, 144, 147, 148,
                                                   149, 150, 151, 152, 153, 154, 155, 156,
                                                   157, 158, 159, 160, 162, 163, 164, 165,
                                                   166, 167, 168, 169, 170, 171, 172, 173,
                                                   174, 175, 176, 178, 179, 180, 181, 182,
                                                   183, 184, 185, 186, 187, 188, 189, 190,
                                                   191, 192, 194, 195, 196, 197, 198, 199,
                                                   200, 201, 202, 203, 204, 205, 206, 207,
                                                   208, 210, 211, 212, 213, 214, 215, 216,
                                                   217, 218, 219, 220, 221, 222, 223, 224,
                                                   226, 227, 228, 229, 230, 231, 232, 233,
                                                   234, 235, 236, 237, 238, 239, 242, 243,
                                                   244, 245, 246, 247, 248, 249, 250, 251,
                                                   252, 253, 254, 255};
      ResetEntries(sizeof(val_ac_chrominance));
      
      memcpy(m_ucLengths,bits_ac_chrominance,sizeof(bits_ac_chrominance));
      memcpy(m_pucValues,val_ac_chrominance ,sizeof(val_ac_chrominance));
      return;
    }   
    break;  
  case Progressive:
    if (depth == 8) {
      static const UBYTE bits_ac_chrominance[] = { 0,3,0,1,2,4,4,3,4,5,4,4,3,2,4,133};
      static const UBYTE val_ac_chrominance[]  = { 0, 1, 17,
                                                   33,
                                                   16, 49,
                                                   2, 32, 65, 81,
                                                   3, 18, 48, 97,
                                                   64, 113, 129,
                                                   34, 80, 145, 161,
                                                   4, 19, 50, 96, 177,
                                                   112, 193, 209, 240,
                                                   20, 51, 66, 225,
                                                   35, 128, 241,
                                                   82, 114,
                                                   5, 52, 98, 144,
                                                   6, 7, 8, 9, 10, 21, 22, 23,
                                                   24, 25, 26, 36, 37, 38, 39, 40,
                                                   41, 42, 53, 54, 55, 56, 57, 58,
                                                   67, 68, 69, 70, 71, 72, 73, 74,
                                                   83, 84, 85, 86, 87, 88, 89, 90,
                                                   99, 100, 101, 102, 103, 104, 105, 106,
                                                   115, 116, 117, 118, 119, 120, 121, 122,
                                                   130, 131, 132, 133, 134, 135, 136, 137,
                                                   138, 146, 147, 148, 149, 150, 151, 152,
                                                   153, 154, 160, 162, 163, 164, 165, 166,
                                                   167, 168, 169, 170, 176, 178, 179, 180,
                                                   181, 182, 183, 184, 185, 186, 192, 194,
                                                   195, 196, 197, 198, 199, 200, 201, 202,
                                                   208, 210, 211, 212, 213, 214, 215, 216,
                                                   217, 218, 224, 226, 227, 228, 229, 230,
                                                   231, 232, 233, 234, 242, 243, 244, 245,
                                                   246, 247, 248, 249, 250}; 
      
      ResetEntries(sizeof(val_ac_chrominance));

      memcpy(m_ucLengths,bits_ac_chrominance,sizeof(bits_ac_chrominance));
      memcpy(m_pucValues,val_ac_chrominance ,sizeof(val_ac_chrominance));
      return;
    } else if (depth == 12) {
      static const UBYTE bits_ac_chrominance[] = { 0, 1, 3, 2, 4, 4, 4, 2, 6, 5, 4, 4, 3, 3, 8, 203};
      static const UBYTE val_ac_chrominance[] =  { 1,
                                                   2, 3, 4,
                                                   5, 17,
                                                   0, 6, 18, 33,
                                                   7, 19, 49, 65,
                                                   8, 34, 81, 97,
                                                   20, 113,
                                                   9, 35, 50, 129, 145, 161,
                                                   21, 66, 177, 193, 240,
                                                   10, 22, 209, 225,
                                                   36, 51, 82, 241,
                                                   11, 23, 98,
                                                   37, 67, 114,
                                                   12, 13, 14, 15, 24, 52, 130, 146,
                                                   16, 25, 26, 27, 28, 29, 30, 31, 32, 38, 39, 40, 41,
                                                   42, 43, 44, 45, 46, 47, 48, 53, 54, 55, 56, 57,
                                                   58, 59, 60, 61, 62, 63, 64, 68, 69, 70, 71,
                                                   72, 73, 74, 75, 76, 77, 78, 79, 80, 83, 84, 85,
                                                   86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 99,
                                                   100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
                                                   110, 111, 112, 115, 116, 117, 118, 119, 120, 121,
                                                   122, 123, 124, 125, 126, 127, 128, 131, 132, 133,
                                                   134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
                                                   144, 147, 148, 149, 150, 151, 152, 153, 154, 155,
                                                   156, 157, 158, 159, 160, 162, 163, 164, 165, 166,
                                                   167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
                                                   178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
                                                   188, 189, 190, 191, 192, 194, 195, 196, 197, 198,
                                                   199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
                                                   210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
                                                   220, 221, 222, 223, 224, 226, 227, 228, 229, 230,
                                                   231, 232, 233, 234, 235, 236, 237, 238, 239, 242,
                                                   243, 244, 245, 246, 247, 248, 249, 250, 251, 252,
                                                   253, 254, 255};
      ResetEntries(sizeof(val_ac_chrominance));
      
      memcpy(m_ucLengths,bits_ac_chrominance,sizeof(bits_ac_chrominance));
      memcpy(m_pucValues,val_ac_chrominance ,sizeof(val_ac_chrominance));
      return;
    }
    break;
  case ACSequential:
  case ACProgressive:
  case Lossless:
  case ACLossless:
  case ACDifferentialSequential:
  case ACDifferentialProgressive:
  case ACDifferentialLossless:
  case ACResidual:
  case ACResidualProgressive:
  case ACResidualDCT:
  case JPEG_LS:
    assert(!"internal coding error - no Huffman table should be required for the selected coding mode");
    break;
  default:
    break;
  }

  ResetEntries(1);
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
void HuffmanTemplate::BuildStatistics(bool fordc)
{
  assert(m_pStatistics == NULL);

  m_pStatistics = new(m_pEnviron) class HuffmanStatistics(fordc);
}
///

/// HuffmanTemplate::BuildDecoder
// Build the huffman decoder given the template data.
void HuffmanTemplate::BuildDecoder(void)
{ 
  ULONG i;
  UBYTE *values;
  ULONG code  = 0;
  UBYTE *symptr;
  UBYTE *sizptr;
  UBYTE **lsbsym;
  UBYTE **lsbsiz;

  assert(m_pDecoder == NULL);

  if (m_pucValues) {
    assert(m_ucLengths);
    // If the decoder is not used, do not build it.
    m_pDecoder = new(m_pEnviron) class HuffmanDecoder(m_pEnviron,
                                                      symptr,sizptr,lsbsym,lsbsiz);
    //
    // Now fill the decoder tables. The tables are indexed by the next 16 bits from
    // the stream (or the next eight bits from the stream) and return the symbol the
    // upper bits of this bitpattern decodes to.
    for(i = 0,values = m_pucValues;i < 16U;i++) {
      // i+1 is the size in bits of the code. 
      // m_ucLength[i] is the number of codes of this size.
      if (m_ucLengths[i]) {
        if (values + m_ucLengths[i] > m_pucValues + m_ulCodewords)
          JPG_THROW(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker","Huffman table marker depends on undefined data");
        //
        // Left-align the code in the table.
        for(UBYTE j = 0;j < m_ucLengths[i];j++) {
          UBYTE symbol = *values++;
          ULONG last   = code + (1 << (15 - i));
          ULONG qcode  = code >> 8;
          ULONG qlast  = last >> 8;
          if (last > MAX_UWORD + 1)
            JPG_THROW(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker",
                      "Huffman table corrupt - entry depends on more bits than available for the bit length");
          // Check whether the code is all-1. This is not allowed.
          if ((code >> (15 - i)) >= (1U << (i + 1)) - 1)
            JPG_WARN(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker",
                     "Found an all-1 Huffman code, this is not permitted. Proceeding anyhow.");
          
          if (i < 8) { // code size is 8 bits or less, note that i + 1 is the code size
            assert(qcode < qlast);
            do {
              symptr[qcode] = symbol;
              sizptr[qcode] = i + 1;
              lsbsym[qcode] = NULL;
              lsbsiz[qcode] = NULL;
            } while(++qcode < qlast);
            code = last;
          } else {
            if (lsbsym[qcode] == NULL) {
              lsbsym[qcode] = (UBYTE *)m_pEnviron->AllocMem(256 * sizeof(UBYTE));
            }
            if (lsbsiz[qcode] == NULL) {
              lsbsiz[qcode] = (UBYTE *)m_pEnviron->AllocMem(256 * sizeof(UBYTE));
              memset(lsbsiz[qcode],0xff,256 * sizeof(UBYTE));
            }
            // Codespace must still be unused or already reserved for the extension
            assert(sizptr[qcode] == 0 || sizptr[qcode] == MAX_UBYTE);
            symptr[qcode] = symbol;
            sizptr[qcode] = 0;
            do {
              lsbsym[qcode][code & 0xff] = symbol;
              lsbsiz[qcode][code & 0xff] = i + 1;
            } while(++code < last);
          }
        }
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
      JPG_THROW(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker","Huffman table marker run out of data");
    m_ucLengths[i] = cnt;
    total         += cnt;
  }
  
  m_ulCodewords = total;
  assert(m_pucValues == NULL);
  m_pucValues = (UBYTE *)m_pEnviron->AllocMem(sizeof(UBYTE) * total);

  for(i = 0;i < total;i++) {
    LONG v = io->Get();
    if (v == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"HuffmanTemplate::ParseMarker","Huffman table marker run out of data");
    m_pucValues[i] = v;
  }
}
///

/// HuffmanTemplate::AdjustToStatistics
// Use the collected statistics to build an optimized
// huffman table.
void HuffmanTemplate::AdjustToStatistics(void)
{
#ifdef COLLECT_STATISTICS
  if (m_pStatistics) {
    FILE *stats;
    char filename[30];
    snprintf(filename,sizeof(filename),"stat_%d_%d_%d.dat",m_bAC,m_bChroma,0);
    
    if ((stats = fopen(filename,"r"))) {
      m_pStatistics->MergeStatistics(stats,m_bAC);
      fclose(stats);
    }
    if ((stats = fopen(filename,"w"))) {
      m_pStatistics->WriteStatistics(stats,m_bAC);
      fclose(stats);
    }
  }
#endif

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
    memset(m_pucValues,0,sizeof(UBYTE) * m_ulCodewords);
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
#ifdef COLLECT_STATISTICS
    {
      FILE *file;
      char filename[30];
      snprintf(filename,sizeof(filename),"lengths_%d_%d_%d.dat",m_bAC,m_bChroma,0);
      int length = m_bAC?256:16;

      if ((file = fopen(filename,"w"))) {
        for(i = 0;i < length;i++) {
          fprintf(file,"%d\n",codesizes[i]);
        }
        fclose(file);
      }
      snprintf(filename,sizeof(filename),"values_%d_%d_%d.dat",m_bAC,m_bChroma,0);
      if ((file = fopen(filename,"w"))) {
        for(i = 0;i < length;i++) {
          fprintf(file,"%d\n",m_pucValues[i]);
        }
      }
    }
#endif
    delete m_pStatistics;
    m_pStatistics = NULL;
  }
}
///
