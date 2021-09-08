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
** This class keeps all the coding tables, huffman, AC table, quantization
** and other side information.
**
** $Id: tables.hpp,v 1.95 2021/09/08 10:30:06 thor Exp $
**
*/

#ifndef CODESTREAM_TABLES_HPP
#define CODESTREAM_TABLES_HPP

/// Include
#include "interface/types.hpp"
#include "tools/environment.hpp"
#include "marker/scantypes.hpp"
#include "boxes/box.hpp"
#include "boxes/databox.hpp"
#include "boxes/namespace.hpp"
#include "boxes/mergingspecbox.hpp"
///

/// Forwards
class ByteStream;
class Quantization;
class QuantizationTable;
class HuffmanTemplate;
class JFIFMarker;
class AdobeMarker;
class ResidualSpecsMarker;
class ColorTrafo;
class EXIFMarker;
class ACTable;
class RestartIntervalMarker;
class NumberOfLinesMarker;
class Thresholds;
class ToneMapperBox;
class ColorTransformerFactory;
class Component;
class Checksum;
class ChecksumBox;
///

/// class Tables
class Tables: public JKeeper {
  //
  // If there is a residual image, here are the settings for it.
  class Tables                  *m_pResidualTables;
  //
  // If this are the tables for the residual tables, here are the
  // main settings.
  class Tables                  *m_pParent;
  //
  // If there is an alpha channel, here are its settings.
  class Tables                  *m_pAlphaTables;
  //
  // In case this is an alpha channel, here is the pointer to the
  // to the image data.
  class Tables                  *m_pMaster;
  //
  // The quantization table.
  class Quantization            *m_pQuant;
  //
  // The huffman table.
  class HuffmanTable            *m_pHuffman;
  //
  // The AC table.
  class ACTable                 *m_pConditioner;
  //
  // The restart interval definition if there is one.
  class RestartIntervalMarker   *m_pRestart;
  //
  // The adobe color marker.
  class AdobeMarker             *m_pColorInfo;
  //
  // The JFIF marker.
  class JFIFMarker              *m_pResolutionInfo;
  //
  // Exif data.
  class EXIFMarker              *m_pCameraInfo;
  //
  // List of all boxes installed in this table.
  // The lifetime of the classes is controlled by
  // this list, not by the individual pointers to the
  // boxes below.
  class Box                     *m_pBoxList;
  //
  // The namespace for searching for boxes. This is the one for
  // the regular image
  class NameSpace                m_NameSpace;
  //
  // And this is the one for the alpha image.
  class NameSpace                m_AlphaNameSpace;
  //
  // The class that builds the color transformers.
  class ColorTransformerFactory *m_pColorFactory;
  //
  // In case we are in the alpha codestream, the data is also
  // hidden in a box, and the alpha stream is here.
  class DataBox                 *m_pAlphaData;
  //
  // The box containing the residual data information
  // for lossless compression.
  class DataBox                 *m_pResidualData;
  //
  // This marker contains the hidden refinement data if
  // this feature is used. This is always attached to the
  // tables (alpha, residual, main) where it is refining.
  class DataBox                 *m_pRefinementData;
  //
  // The color transformer
  class ColorTrafo              *m_pColorTrafo;
  //
  // The thresholds for JPEG LS
  class Thresholds              *m_pThresholds;
  //
  // The extended reversible color transformation information coming
  // from JPEG LS
  class LSColorTrafo            *m_pLSColorTrafo;
  //
  // The merging specifications.
  class MergingSpecBox          *m_pResidualSpecs;
  //
  // The merging specifications for the alpha channel.
  class MergingSpecBox          *m_pAlphaSpecs;
  //
  // An identity tone mapping that is used if no tone mapping marker
  // is present. This is not part of the box list because it is
  // never written to disk.
  class Box                     *m_pIdentityMapping;
  //
  // The checksum box (once loaded), only here on parsing, not on writing.
  class ChecksumBox             *m_pChecksumBox;
  //
  // The maximum error bound.
  UBYTE                          m_ucMaxError;
  //
  // Indicator whether the color transformation should only
  // code residuals if the LDR domain is out of range.
  bool                           m_bTruncateColor;
  //
  // True in case refinement data is to be written or read.
  bool                           m_bRefinement;
  //
  //
  // True in case this is an openloop encoder that uses the original
  // image and not the reconstructed image for the precursor.
  bool                           m_bOpenLoop;
  //
  // True in case the deadzone quantizer shall be used on encoding.
  // Otherwise, the default equi-quantizer is used.
  bool                           m_bDeadZone;
  //
  // True in case a quantization optimization of the encoder/quantizer
  // is desired.
  bool                           m_bOptimize;
  //
  // True in case the de-ringing filter on encoding is enabled.
  bool                           m_bDeRing;
  //
  // This flag is set if an exp marker is found in the tables.
  bool                           m_bFoundExp;
  //
  // The following two flags indicate the horizontal and vertical
  // expansion flags for the exp marker (if present)
  bool                           m_bHorizontalExpansion;
  bool                           m_bVerticalExpansion;
  //
  // Build a tone mapping for the type (base-tag) and the given tag list
  // The base tag is the tag-id for the type of the box. All others are offsets.
  class ToneMapperBox *BuildToneMapping(const struct JPG_TagItem *tags,
                                        JPG_Tag basetag,UBYTE inbits,UBYTE outbits);
  //
  // Scan for a refinement box in the box list of this tables class.
  // Return the box if it is found, or NULL if no refinement exists. Note that the
  // box list is maintained by the top-level tables that applies to the legacy
  // codestream.
  class DataBox *RefinementDataOf(UWORD index,ULONG boxtype) const;
  //
  //
  // Parse off the tags for a profile C encoder
  void CreateProfileCSettings(const struct JPG_TagItem *tags,class FileTypeBox *profile,
                              UBYTE precison,UBYTE rangebits,
                              MergingSpecBox::DecorrelationType ltrafo,bool dopart8,bool dopart9);
  //
  // Return the merging spec box responsible for this table.
  class MergingSpecBox *SpecsOf(void) const;
  //
public:
  Tables(class Environ *env);
  //
  ~Tables(void);
  //
  // Create residual tables for the side channel. The
  // life time of the tables is not allocated here, only
  // the linkage is established.
  class Tables *CreateResidualTables(void);
  //
  // Create the tables for an alpha channel. This be
  class Tables *CreateAlphaTables(void);
  //
  // Return the regular name space, namely that of the image.
  class NameSpace *ImageNamespace(void);
  //
  // Return the common name space for the alpha channel.
  class NameSpace *AlphaNamespace(void);
  //
  // Parse off tables, including an application marker,
  // comment, huffman tables or quantization tables.
  // If the checksum is non-NULL, all data except boxes and comments
  // are checksummed.
  void ParseTables(class ByteStream *io,class Checksum *chk,
                   bool allowexp,bool isls);
  //
  // Prepare reading an incremental part of the tables. This here must be called first
  // before continuing with one or multiple ParseTableIncremental calls.
  void ParseTablesIncrementalInit(bool allowexp);
  //
  // Read an incremental part of the tables, namely the next marker.
  // Returns true in case the tables/misc section is not yet complete,
  // and this function should be called again. Returns false in case
  // the tables/misc section is complete.
  bool ParseTablesIncremental(class ByteStream *io,class Checksum *chk,
                              bool allowexp,bool isls);
  //
  // Write the tables to the codestream.
  void WriteTables(class ByteStream *io);
  //
  // For writing, install the standard suggested tables.
  // Create tables for the residuals if this flag is set.
  // precision is the overall bit-precision of the image, rangebits the
  // number of extra bits hidden in residual scans.
  void InstallDefaultTables(UBYTE precision,UBYTE rangebits,const struct JPG_TagItem *tags);
  //
  // Find the DC huffman table of the indicated index.
  class HuffmanTemplate *FindDCHuffmanTable(UBYTE idx,ScanType type,
                                            UBYTE depth,UBYTE hidden,UBYTE scanidx) const;
  //
  // Find the AC huffman table of the indicated index.
  class HuffmanTemplate *FindACHuffmanTable(UBYTE idx,ScanType type,
                                            UBYTE depth,UBYTE hidden,UBYTE scanidx) const;
  //
  // Find the AC conditioner table for the indicated index
  // and the DC band.
  class ACTemplate *FindDCConditioner(UBYTE idx,ScanType type,
                                      UBYTE depth,UBYTE hidden,UBYTE scanidx) const;
  //
  // The same for the AC band.
  class ACTemplate *FindACConditioner(UBYTE idx,ScanType type,
                                      UBYTE depth,UBYTE hidden,UBYTE scanidx) const;
  //
  // Find the quantization table of the given index.
  class QuantizationTable *FindQuantizationTable(UBYTE idx) const;
  //
  // Return the residual data if any.
  class DataBox *ResidualDataOf(void) const
  {
    if (m_pParent)
      return m_pParent->m_pResidualData;
    
    return m_pResidualData;
  }
  //
  // Return the alpha codestream if there is any. Note that the
  // residual data of the alpha codestream can be obtained by
  // using CreateAlphaTables()->ResidualDataOf().
  class DataBox *AlphaDataOf(void) const
  {
    if (m_pMaster) {
      // This is the alpha tables codestream.
      return m_pAlphaData;
    } else if (m_pAlphaTables) {
      // In the master codestream. Go to the alpha tables.
      return m_pAlphaTables->m_pAlphaData;
    }
    return NULL;
  }
  //
  // Return the n'th refinement data if any.
  class DataBox *RefinementDataOf(UWORD index) const;
  //
  // Append a new refinement box on creating refinement scans.
  class DataBox *AppendRefinementData(void);
  //
  // Return the thresholds of JPEG LS or NULL if there are none.
  class Thresholds *ThresholdsOf(void) const
  {
    return m_pThresholds;
  }
  //
  // Find the tone mapping box of the given table index, or NULL
  // if this box is missing.
  class ToneMapperBox *FindToneMapping(UBYTE tabidx) const
  {
    if (m_pMaster) {
      return m_pMaster->m_AlphaNameSpace.FindNonlinearity(tabidx);
    } else {
      return m_NameSpace.FindNonlinearity(tabidx);
    }
  }
  //
  // Find the transformation matrix of the given matrix decorrelation type. This
  // works of course only for freeform transformations as otherwise the matrix box
  // is not required.
  class MatrixBox *FindMatrix(MergingSpecBox::DecorrelationType dt) const
  {
    if (m_pMaster) {
      return m_pMaster->m_AlphaNameSpace.FindMatrix(dt);
    } else {
      return m_NameSpace.FindMatrix(dt);
    }
  }
  //
  // Return the color transformer suitable for the external data
  // type and the color space indicated in the application markers.
  class ColorTrafo *ColorTrafoOf(class Frame *frame,class Frame *residualframe,
                                 UBYTE external_type,bool encoding,bool disabletorgb); 
  //
  // Check whether residual data in the APP11 marker shall be written.
  bool UseResiduals(void) const;
  //
  // Check whether the refinement data in APP11 shall be written.
  bool UseRefinements(void) const;
  //
  // Check whether to use the lossless DCT transformation for the given
  // component index.
  bool UseLosslessDCT(void) const;
  //
  // Check whether the lossless flag is set.
  bool isLossless(void) const;
  //
  // Return a flag that indicates whether chroma samples are
  // centered or cosited. Returns true if they are cosited.
  bool isChromaCentered(void) const;
  //
  // Return a flag indicating whether the downsampler at encoder
  // side should enable interpolation (then true) or if a simple
  // box filter is sufficient (then false).
  // Currently, residual coding requires the box filter.
  bool isDownsamplingInterpolated(void) const;
  //
  // Return the maximal masking error.
  UBYTE MaxErrorOf(void) const
  {
    return m_ucMaxError;
  }
  //
  // Return an indicator whether the encoder should use the original
  // signal (instead of the quantized signal) to implement an
  // open loop encoding.
  bool isOpenLoop(void) const
  {
    return m_bOpenLoop;
  }
  //
  // Return an indicator whether these tables are the residual
  // tables or the main (legacy) tables.
  bool isResidualTable(void) const
  {
    return (m_pParent == NULL)?false:true;
  }
  //
  // Check how many fractional bits the color transformation will use.
  // This is either from the L or the R transformation depending on
  // which tables this is (residual or legacy). The DCT flag indicates
  // whether a DCT is in the path. If so, more bits might be allocated
  // to accomodate fractional output bits of the DCT. Note that this
  // is an implementation detail.
  UBYTE FractionalColorBitsOf(UBYTE count,bool dct) const;
  //
  // Return the number of fractional bits in the L-path.
  UBYTE FractionalLBitsOf(UBYTE count,bool dct) const;
  //
  // Return the number of fractional bits in the R-path.
  UBYTE FractionalRBitsOf(UBYTE count,bool dct) const;
  //
  // Check how many bits are hidden in invisible refinement scans.
  UBYTE HiddenDCTBitsOf(void) const;
  //
  // Test whether this setup has designated chroma components. For the
  // legacy codestream, this tests whether there is an L transformation in
  // the path. For the residual codestream, this tests for an R-transformation.
  bool hasSeparateChroma(UBYTE depth) const;
  //
  // Build the proper DCT transformation for the specification
  // recorded in this class. The DCT is not owned by this class
  // and must be deleted by the caller.
  class DCT *BuildDCT(class Component *comp,UBYTE count,
                      UBYTE precision) const;
  //
  // Return the currently active restart interval in MCUs or zero
  // in case restart markers are disabled.
  ULONG RestartIntervalOf(void) const;
  //
  // Return the effective color transformation for the L-transformation.
  // The argument is the number of components.
  MergingSpecBox::DecorrelationType LTrafoTypeOf(UBYTE components) const;
  //
  // Return the effective color transformation for the R-transformation.
  // The argument is the number of components.
  MergingSpecBox::DecorrelationType RTrafoTypeOf(UBYTE components) const;
  //
  // Return the effective color transformation for C.
  MergingSpecBox::DecorrelationType CTrafoTypeOf(UBYTE components) const;
  //
  // Find the merging specifications.
  class MergingSpecBox *ResidualSpecsOf(void) const
  {
    if (m_pMaster) {
      return m_pMaster->m_pAlphaSpecs;
    } else if (m_pParent) {
      return m_pParent->m_pResidualSpecs;
    } else {
      return m_pResidualSpecs;
    }
  }
  //
  // Find the alpha merging specification, or NULL in case we do not have
  // any alpha here.
  class MergingSpecBox *AlphaSpecsOf(void) const
  {
    if (m_pMaster) {
      return m_pMaster->m_pAlphaSpecs;
    } else {
      return m_pAlphaSpecs;
    }
  }
  //
  // Find the checksum box if there is one.
  class ChecksumBox *ChecksumOf(void) const
  {
    if (m_pParent)
      return m_pParent->m_pChecksumBox;
    
    return m_pChecksumBox;
  }
  //
  // Return an indicator whether the checksum includes the markers.
  // This is not the case for part-3 and following.
  bool ChecksumTables(void) const
  {
    return false;
  }
  //
  // Return the JPEG-LS part-2 color specification if there is one.
  const class LSColorTrafo *LSColorTrafoOf(void) const
  {
    return m_pLSColorTrafo;
  }
  //
  // Return an indicator whether an exp marker has been found. If so,
  // return the h and v expansion flags.
  bool isEXPDetected(bool &h,bool &v) const
  {
    h = m_bHorizontalExpansion;
    v = m_bVerticalExpansion;
    
    return m_bFoundExp;
  }
  //
  // Returns true in case the quantization optimization is desired.
  bool Optimization() const
  {
    return m_bOptimize;
  }
  //
  // Returns true if the optional deringing filter is enabled. This works
  // only for the LDR image (plus refinement).
  bool isDeringingEnabled(void) const
  {
    return m_bDeRing;
  }
};
///

///
#endif
