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
 * Definition of the bitmap hook class.
 * This hook function is used to pull data out of the
 * client to make it available to the jpeglib
 * 
 * $Id: bitmaphook.hpp,v 1.10 2015/03/11 16:02:42 thor Exp $
 *
 */

#ifndef BITMAPHOOK_HPP
#define BITMAPHOOK_HPP

/// Includes
#include "interface/types.hpp"
#include "interface/tagitem.hpp"
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "imagebitmap.hpp"
///

/// Forwards
struct JPG_Hook;
///

/// Design
/** Design
******************************************************************
** class BitMapHook                                             **
** Super Class: none                                            **
** Sub Classes: none                                            **
** Friends:     none                                            **
******************************************************************

Using a JPG_Hook as its main ingredience, the bitmap hook defines
a call-back entry point that is called by the jpeglib as soon as
it either requires input data on encoding, or as it generated output
data to be placed into the client on decoding.

The required position information, i.e. which data has to be
delivered by the client, or which data is ready to be placed into
the client application, leaves the hook as a tag list that is
to be parsed by the client.

The client then has to deliver information where the image data has
to be placed, as the memory start position of the canvas, the
data type the pixels are stored within, the number of bytes each
pixel is wide, i.e. how many bytes one has to add to the address
of the current pixel to get to the pixel to the right, and the
modulo value, i.e. how many bytes one has to add to get to the
address of the pixel below the current pixel.

One special note about the pixel addressing, though: What the
bitmap hook expects as start address is always the base address
of the canvas, i.e. of pixel 0,0. Whether this pixel is actually
used by the current request is a different matter.

The library uses this pixel address, adds bytesperpixel * xmin
and bytesperrow * ymin to get the pixel address it is really
interested in. 

From the libraries point of view, the bitmap hook class
delivers this output data by means of an "ImageBitMap" structure.

* */
///

/// The bitmap hook class
// This class is mainly intented to pull bitmap type data out of the
// client.
class BitMapHook : public JObject {
  //
  // The function to call for reading the HDR data.
  // Setup by ParseTags.
  struct JPG_Hook   *m_pHook; 
  // 
  // The function to call for retrieving the LDR data. This is
  // optional, tone mapping may be performed within the library code.
  struct JPG_Hook   *m_pLDRHook;
  //
  // The function to call for retrieving opacity data. This is
  // only required if there is opacity data.
  struct JPG_Hook   *m_pAlphaHook;
  //
  // The following keep default data for lazy applications:
  struct ImageBitMap m_DefaultImageLayout;
  //
  // Prepared tags for requesting usual bitmap tags.
  struct JPG_TagItem m_BitmapTags[25];
  //
  // Prepared tags for the LDR image request
  struct JPG_TagItem m_LDRTags[25];
  //
  // Setup the input parameter tags for the user hook.
  void InitHookTags(struct JPG_TagItem *tags);
  //
  // Fill the tag items for a request call and make the call.
  void Request(struct JPG_Hook *hook,struct JPG_TagItem *tags,UBYTE pixeltype,
               const RectAngle<LONG> &rect,struct ImageBitMap *ibm,const class Component *comp,bool alpha);
  //
  // Release the tag items for a release call and make the call.
  void Release(struct JPG_Hook *hook,struct JPG_TagItem *tags,UBYTE pixeltype,
               const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,const class Component *comp,bool alpha);
  //
public:
  //
  // Build a bitmap hook from a tag list (user parameters)
  // and the address for which this data is required.
  BitMapHook(const struct JPG_TagItem *tags);
  //
  // Only re-parse the tags to re-define the default bitmap layout.
  void ParseTags(const struct JPG_TagItem *tags);
  //
  // Collect data from the user.
  void RequestClientData(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
                         const class Component *comp);
  //
  // Release the image portion from the client, called to signal the client
  // that all has gone well.
  void ReleaseClientData(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
                         const class Component *comp);
  //
  // Collect alpha channel (opacity) data from the user, either to request the input
  // opacity on encoding, or to request a buffer where the alpha data is placed
  // when decoding. Note that you cannot define dedicated LDR data for alpha. It is
  // always automatically generated with the alpha "tone mapper".
  void RequestClientAlpha(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
                          const class Component *comp);
  //
  // Release the opacity information again. On decoding, this means that opacity is now
  // ready to be used. On encoding it means that the encoder has processed the data.
  void ReleaseClientAlpha(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
                          const class Component *comp);
  //
  // Check whether an LDR image is available (returns true) or whether
  // the caller has to do the tonemapping itself (returns false).
  bool providesLDRImage(void) const
  {
    return (m_pLDRHook)?(true):(false);
  }
  //
  // Retrieve the LDR tone mapped version of the user. This requires that an
  // LDR hook function is available, i.e. should only be called if the 
  // providesLDRImage() method above returns true.
  void RequestLDRData(const RectAngle<LONG> &rect,struct ImageBitMap *ibm,
                      const class Component *comp);
  //
  // Release the requested LDR data. Requires that an LDR hook is available, i.e.
  // providesLDRImage() must have been checked before and must have returned
  // true for this to make sense.
  void ReleaseLDRData(const RectAngle<LONG> &rect,const struct ImageBitMap *ibm,
                      const class Component *comp);
};
///

///
#endif
