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
** This class represents the quantization tables.
**
** $Id: quantization.cpp,v 1.37 2021/11/15 07:39:43 thor Exp $
**
*/

/// Includes
#include "marker/quantization.hpp"
#include "marker/quantizationtable.hpp"
#include "io/bytestream.hpp"
#include "dct/dct.hpp"
///

/// Pre-defined quantization tables 
static const LONG std_luminance_quant_tbl[64] = {
  16,  11,  10,  16,  24,  40,  51,  61,
  12,  12,  14,  19,  26,  58,  60,  55,
  14,  13,  16,  24,  40,  57,  69,  56,
  14,  17,  22,  29,  51,  87,  80,  62,
  18,  22,  37,  56,  68, 109, 103,  77,
  24,  35,  55,  64,  81, 104, 113,  92,
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99
};
static const LONG std_chrominance_quant_tbl[64] = {
  17,  18,  24,  47,  99,  99,  99,  99,
  18,  21,  26,  66,  99,  99,  99,  99,
  24,  26,  56,  99,  99,  99,  99,  99,
  47,  66,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99
};
static const LONG flat_luminance_tbl[64] = { // This also goes for luma, obviously...
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16
};
static const LONG ssim_luminance_tbl[64] = {
  12,  17,  20,  21,  30,  34,  56,  63,
  18,  20,  20,  26,  28,  51,  61,  55,
  19,  20,  21,  26,  33,  58,  69,  55,
  26,  26,  26,  30,  46,  87,  86,  66,
  31,  33,  36,  40,  46,  96, 100,  73,
  40,  35,  46,  62,  81, 100, 111,  91,
  46,  66,  76,  86, 102, 121, 120, 101,
  68,  90,  90,  96, 113, 102, 105, 103
}; 
static const LONG ssim_chrominance_tbl[64] = {
    8,  12,  15,  15,  86,  96,  96,  98,
   13,  13,  15,  26,  90,  96,  99,  98,
   12,  15,  18,  96,  99,  99,  99,  99,
   17,  16,  90,  96,  99,  99,  99,  99,
   96,  96,  99,  99,  99,  99,  99,  99,
   99,  99,  99,  99,  99,  99,  99,  99,
   99,  99,  99,  99,  99,  99,  99,  99,
   99,  99,  99,  99,  99,  99,  99,  99
};
static const LONG imagemagick_luminance_tbl[64] = { // This is also used for chroma.
  16,  16,  16,  18,  25,  37,  56,  85,
  16,  17,  20,  27,  34,  40,  53,  75,
  16,  20,  24,  31,  43,  62,  91,  135,
  18,  27,  31,  40,  53,  74,  106, 156,
  25,  34,  43,  53,  69,  94,  131, 189,
  37,  40,  62,  74,  94,  124, 169, 238,
  56,  53,  91,  106, 131, 169, 226, 311,
  85,  75,  135, 156, 189, 238, 311, 418
};
static const LONG hvs_luminance_tbl[64] = {
    9,  10,  12,  14,  27,  32,  51,  62,
   11,  12,  14,  19,  27,  44,  59,  73,
   12,  14,  18,  25,  42,  59,  79,  78,
   17,  18,  25,  42,  61,  92,  87,  92,
   23,  28,  42,  75,  79, 112, 112,  99,
   40,  42,  59,  84,  88, 124, 132, 111,
   42,  64,  78,  95, 105, 126, 125,  99,
   70,  75, 100, 102, 116, 100, 107,  98
}; 
static const LONG hvs_chrominance_tbl[64] = {
      9,  10,  17,  19,  62,  89,  91,  97,
     12,  13,  18,  29,  84,  91,  88,  98,
     14,  19,  29,  93,  95,  95,  98,  97,
     20,  26,  84,  88,  95,  95,  98,  94,
     26,  86,  91,  93,  97,  99,  98,  99,
     99, 100,  98,  99,  99,  99,  99,  99,
     99,  99,  99,  99,  99,  99,  99,  99,
     97,  97,  99,  99,  99,  99,  97,  99
};
static const LONG klein_luminance_tbl[64] = { // this is also used for chroma
   10,  12,  14,  19,  26,  38,  57,  86,
   12,  18,  21,  28,  35,  41,  54,  76,
   14,  21,  25,  32,  44,  63,  92, 136,
   19,  28,  32,  41,  54,  75, 107, 157,
   26,  35,  44,  54,  70,  95, 132, 190,
   38,  41,  63,  75,  95, 125, 170, 239,
   57,  54,  92, 107, 132, 170, 227, 312,
   86,  76, 136, 157, 190, 239, 312, 419
};
static const LONG dctune_luminance_tbl[64] = { // this is also used for chroma
    7,   8,  10,  14,  23,  44,  95, 241,
    8,   8,  11,  15,  25,  47, 102, 255,
   10,  11,  13,  19,  31,  58, 127, 255,
   14,  15,  19,  27,  44,  83, 181, 255,
   23,  25,  31,  44,  72, 136, 255, 255,
   44,  47,  58,  83, 136, 255, 255, 255,
   95, 102, 127, 181, 255, 255, 255, 255,
  241, 255, 255, 255, 255, 255, 255, 255
};
static const LONG ahumada1_luminance_tbl[64] = { // this is also used for chroma
   15,  11,  11,  12,  15,  19,  25,  32,
   11,  13,  10,  10,  12,  15,  19,  24,
   11,  10,  14,  14,  16,  18,  22,  27,
   12,  10,  14,  18,  21,  24,  28,  33,
   15,  12,  16,  21,  26,  31,  36,  42,
   19,  15,  18,  24,  31,  38,  45,  53,
   25,  19,  22,  28,  36,  45,  55,  65,
   32,  24,  27,  33,  42,  53,  65,  77
};
static const LONG ahumada2_luminance_tbl[64] = { // this is also used for chroma
   14,  10,  11,  14,  19,  25,  34,  45,
   10,  11,  11,  12,  15,  20,  26,  33,
   11,  11,  15,  18,  21,  25,  31,  38,
   14,  12,  18,  24,  28,  33,  39,  47,
   19,  15,  21,  28,  36,  43,  51,  59,
   25,  20,  25,  33,  43,  54,  64,  74,
   34,  26,  31,  39,  51,  64,  77,  91,
   45,  33,  38,  47,  59,  74,  91, 108
};
///

/// Quantization::Quantization
Quantization::Quantization(class Environ *env)
  : JKeeper(env)
{
  memset(m_pTables,0,sizeof(m_pTables));
}
///

/// Quantization::~Quantization
Quantization::~Quantization(void)
{
  int i;

  for(i = 0;i < 4;i++) {
    delete m_pTables[i];
  }
}
///


/// Quantization::WriteMarker
// Write the DQT marker to the stream.
void Quantization::WriteMarker(class ByteStream *io)
{
  int i,j;
  UWORD len = 2; // The marker size
  UBYTE types[4];

  for(i = 0;i < 4;i++) {
    types[i] = 0;

    if (m_pTables[i]) {
      const UWORD *delta = m_pTables[i]->DeltasOf();
      for(j = 0;j < 64;j++) {
        if (delta[j] > 255) {
          types[i] = 1;
          len     += 64;
          break;
        }
      }
      len += 64 + 1;
    }
  }
  
  io->PutWord(len);
  for(i = 0;i < 4;i++) {
    if (m_pTables[i]) {
      const UWORD *delta = m_pTables[i]->DeltasOf();
      io->Put((types[i] << 4) | i);
      if (types[i]) {
        for(j = 0;j < 64;j++) {
          io->PutWord(delta[DCT::ScanOrder[j]]);
        }
      } else {
        for(j = 0;j < 64;j++) {
          io->Put(delta[DCT::ScanOrder[j]]);
        }
      }
    }
  }
}
///

/// Quantization::InitDefaultTables
// Initialize the quantization table to the standard example
// tables for quality q, q=0...100
// If "addresidual" is set, additional quantization tables for 
// residual coding are added into the legacy quantization matrix.
// If "foresidual" is set, the quantization table is for the residual
// codestream, using the hdrquality parameter (with known ldr parameters)
// but injected into the residual codestream.
// If "rct" is set, the residual color transformation is the RCT which
// creates one additional bit of precision for lossless. In lossy modes,
// this bit can be stripped off.
// The table selector argument specifies which of the build-in
// quantization table to use. CUSTOM is then a pointer to a custom
// table if the table selector is custom.
void Quantization::InitDefaultTables(UBYTE quality,UBYTE hdrquality,bool colortrafo,
                                     bool addresidual,bool forresidual,bool rct,
                                     LONG tableselector,UBYTE precision,
                                     const LONG customluma[64],
                                     const LONG customchroma[64])
{
  int i,j;
  UWORD deltas[64];
  const LONG *table       = NULL;
  const LONG *lumatable   = NULL;
  const LONG *chromatable = NULL;
  int scale;
  int hdrscale;
 
  if (quality < 1)
    quality = 1;
  if (quality > 100)
    quality = 100;

  if (quality < 50)
    scale = 5000 / quality;
  else
    scale = 200 - quality * 2;

  if (addresidual || forresidual) {
    if (hdrquality == 0) {
      hdrscale = MAX_UWORD;
    } else if (hdrquality >= 100) {
      hdrscale = 0; // maxed out
    } else {
      int hdrq = hdrquality; //((hdrquality << 2) - hdrquality + quality) >> 2;
      if (hdrq == 0) {
        hdrscale = MAX_UWORD;
      } else if (hdrq < 50) {
        hdrscale = 5000 / hdrq;
      } else if (hdrq <= 100) {
        hdrscale = 200 - hdrq * 2;
      } else {
        hdrscale = 0;
      }
    }
  } else {
    hdrscale = -1;
  }

  //
  // Select now the table source.
  switch(tableselector) {
  case JPGFLAG_QUANTIZATION_ANNEX_K:
    lumatable   = std_luminance_quant_tbl;
    chromatable = std_chrominance_quant_tbl;
    break;
  case JPGFLAG_QUANTIZATION_FLAT:
    lumatable   = flat_luminance_tbl;
    chromatable = flat_luminance_tbl;
    break;
  case JPGFLAG_QUANTIZATION_SSIM:
    lumatable   = ssim_luminance_tbl;
    chromatable = ssim_chrominance_tbl;
    break;
  case JPGFLAG_QUANTZATION_IMAGEMAGICK:
    lumatable   = imagemagick_luminance_tbl;
    chromatable = imagemagick_luminance_tbl;
    break;
  case JPGFLAG_QUANTIZATION_HVS:
    lumatable   = hvs_luminance_tbl;
    chromatable = hvs_chrominance_tbl;
    break;
  case JPGFLAG_QUANTIZATION_KLEIN:
    lumatable   = klein_luminance_tbl;
    chromatable = klein_luminance_tbl;
    break;
  case JPGFLAG_QUANTIZATION_DCTUNE:
    lumatable   = dctune_luminance_tbl;
    chromatable = dctune_luminance_tbl;
    break;
  case JPGFLAG_QUANTIZATION_AHUMADA1:
    lumatable   = ahumada1_luminance_tbl;
    chromatable = ahumada1_luminance_tbl;
    break;
  case JPGFLAG_QUANTIZATION_AHUMADA2:
    lumatable   = ahumada2_luminance_tbl;
    chromatable = ahumada2_luminance_tbl;
    break;
  case JPGFLAG_QUANTIZATION_CUSTOM:
    if (customluma == NULL)
      JPG_THROW(MISSING_PARAMETER,"Quantization::InitDefaultTables",
                "custom quantization has been specified, but the custom luma quantization matrix is not present");
    lumatable   = customluma;
    if (customchroma == NULL)
      chromatable = lumatable;
    else
      chromatable = customchroma;
    break;
  default:
    JPG_THROW(INVALID_PARAMETER,"Quantization::InitDefaultTables",
              "an invalid quantization matrix type has been specified");
    break;
  }

  //
  // There are only two tables populated by default, which is consistent
  // with baseline requirements.
  for(i = 0;i < 4;i++) {
    switch(i) {
    case 0:
      table = lumatable;
      break;
    case 1:
      if (colortrafo) {
        table = chromatable;
      } else {
        table = NULL;
      }
      break;
    default:
      table = NULL;
    }
    if (table) {
      for(j = 0;j < 64;j++) {
        LONG mult  = (i >= 2 || forresidual)?(hdrscale):(scale);
        LONG delta = (table[j] * mult  + 50) / 100;
        LONG lelta = (table[j] * scale + 50) / 100;
#if BETTER_QUANTIZATION
        if ((j & 7) + (j >> 3) > 3) {
          delta = (table[j] * mult + 50) / 100;
        } else if ((j & 7) + (j >> 3) > 2) {
          int sc = (mult > 400)?(400):mult;
          delta  = (table[j] * sc + 50) / 100;
        } else if ((j & 7) + (j >> 3) >= 1) {
          int sc = (mult > 200)?(200):mult;
          delta  = (table[j] * sc + 50) / 100;
        } else { 
          int sc = (mult > 100)?(100):mult;
          delta  = (table[j] * sc + 50) / 100;
        }

        if ((j & 7) + (j >> 3) > 3) {
          lelta = (table[j] * scale + 50) / 100;
        } else if ((j & 7) + (j >> 3) > 2) {
          LONG sc = (scale > 400)?(400):scale;
          lelta  = (table[j] * sc + 50) / 100;
        } else if ((j & 7) + (j >> 3) >= 1) {
          LONG sc = (scale > 200)?(200):scale;
          lelta  = (table[j] * sc + 50) / 100;
        } else { 
          LONG sc = (scale > 100)?(100):scale;
          lelta  = (table[j] * sc + 50) / 100;
        }
#endif
        // Quantize at least as fine as the base layer,
        // otherwise no reasonable residual can be expected.
        if (false && forresidual && delta > lelta && delta > 0)
          delta = lelta;
        if (delta <= 0) 
          delta = 1;
        if (delta > 32767)
          delta = 32767;
        if (rct && (forresidual && i == 1) && delta > 1) {
          // The range of the chroma components was extended by one extra bit.
          delta <<= 1;
        }
        if (rct && (forresidual && i == 0)) {
          // Luma is also extended by one bit, can even be stripped off for lossless.
          delta <<= 1;
        }
        // thor fix: if the quantization tables are part of an 8-bit stream,
        // the table entries shall be byte-sized.
        if (precision < 12 && delta > 255)
          delta = 255;
        deltas[j] = delta;
      }
      if (m_pTables[i] == NULL)
        m_pTables[i] = new(m_pEnviron) class QuantizationTable(m_pEnviron);     
      m_pTables[i]->DefineBucketSizes(deltas);
    } else {
      delete m_pTables[i];
      m_pTables[i] = NULL;
    }
  }
}
///

/// Quantization::ParseMarker
void Quantization::ParseMarker(class ByteStream *io)
{
  LONG len = io->GetWord();

  if (len < 2)
    JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker must be at least two bytes long");

  len -= 2; // remove the marker length.

  while(len > 2) {
    UWORD deltas[64];
    LONG type   = io->Get();
    LONG target;
    int i;

    if (type == ByteStream::EOF)
      JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker run out of data");

    target = (type & 0x0f);
    type >>= 4;
    if (type != 0 && type != 1)
      JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker entry type must be either 0 or 1");
    if (target < 0 || target > 3)
      JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker target table must be between 0 and 3");

    len -= 1; // remove the byte.

    if (type == 0) {
      // UBYTE entries.
      if (len < 64)
        JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker contains insufficient data");
      //
      for(i = 0;i < 64;i++) {
        LONG data = io->Get();
        if (data == ByteStream::EOF)
          JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker run out of data");
        deltas[DCT::ScanOrder[i]] = data;
      }
      len -= 64;
    } else {
      assert(type == 1);
      
      if (len < 64 * 2)
        JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker contains insufficient data");
      //
      for(i = 0;i < 64;i++) {
        LONG data = io->GetWord();
        if (data == ByteStream::EOF)
          JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker run out of data");
        deltas[DCT::ScanOrder[i]] = data;
      }
      len -= 64 * 2;
    }
    //
    // For multiple tables the current table is replaced by the new table. Interestingly, this
    // shall be supported according to the specs.
    if (m_pTables[target] == NULL)
      m_pTables[target] = new(m_pEnviron) class QuantizationTable(m_pEnviron);
    m_pTables[target]->DefineBucketSizes(deltas);
  }

  if (len != 0)
    JPG_THROW(MALFORMED_STREAM,"Quantization::ParseMarker","DQT marker size corrupt");
}
///
