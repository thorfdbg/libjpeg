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
 * The following file describes various tag items provided for the
 * client application to define all the parameters the jpeg library
 * needs. 
 *
 * $Id: parameters.hpp,v 1.26 2012-09-28 17:05:21 thor Exp $
 *
 * These parameters are specified by the user upon invocation of
 * the library.
 * Note that due to the construction of tag lists, it is legal to
 * drop most of the parameters as well.
 * In this case, the library will pick useful defaults, or defaults
 * it considers as useful, whatever this would mean for you.
 */

#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

/// Includes
#include "jpgtypes.hpp"
#include "tagitem.hpp"
///

/// Defines
#define JPG_MAKEID(a,b,c,d) (((JPG_ULONG(a))<<24) | ((JPG_ULONG(b))<<16) | ((JPG_ULONG(c))<<8) | ((JPG_ULONG(d))))
///

/// Parameters defining the image layout and coding parameters.
#define JPGTAG_IMAGE_BASE     (JPGTAG_TAG_USER + 0x200)
//
// Define the dimensions of the image in pixel width and height.
//
// Width of the image in pixels
#define JPGTAG_IMAGE_WIDTH    (JPGTAG_IMAGE_BASE + 0x01)
//
// Height of the image in pixels.
#define JPGTAG_IMAGE_HEIGHT   (JPGTAG_IMAGE_BASE + 0x02)
//
// Depth of the image in components. Valid are values between 1 and 256,
// though progressive only supports 1 to 4.
#define JPGTAG_IMAGE_DEPTH    (JPGTAG_IMAGE_BASE + 0x03)
//
// Precision of the components in bits, may vary between 1 and 16
#define JPGTAG_IMAGE_PRECISION (JPGTAG_IMAGE_BASE + 0x04)
//
// Image frame type, defines the encoding mechanism
#define JPGTAG_IMAGE_FRAMETYPE (JPGTAG_IMAGE_BASE + 0x05)
//
// Possible frame types:
//
// standard sequential mode, up to 4 components, reduced quantization
// table precision, max Delta is 255.
#define JPGFLAG_BASELINE 0
//
// Extended baseline, longer quantization tables, up to 256 components.
#define JPGFLAG_SEQUENTIAL 1
//
// Progressive mode, only 4 components, but quality refinement is possible.
#define JPGFLAG_PROGRESSIVE 2
//
// 10918 predictive lossless mode. Also disable the color transformation for
// true lossless.
#define JPGFLAG_LOSSLESS 3
//
// 14495 (JPEG LS) lossless or near-lossless mode. Requires a disabled
// color transformation.
#define JPGFLAG_JPEG_LS 4
//
// Binary-or this to the above flags to have the mode in the arithmetic
// coded version. Note that there is no AC baseline mode.
#define JPGFLAG_ARITHMETIC 8
//
// Binary-or this to the above flags to enable the pyramidal (hierarchical)
// coding scheme. Again, not available for baseline, more tags for defining
// the pyramidal mode below - specifically the number of resolution levels
#define JPGFLAG_PYRAMIDAL 16
//
// Binary-or this to enable the reversible SERMS-based DCT transformation.
// This will allow lossless coding even in the baseline, sequential and
// progressive modes. Also disable the color transformation for true lossless.
#define JPGFLAG_REVERSIBLE_DCT 32
//
// Binary-or this to enable residual coding. This is just another mechanism
// to allow lossless coding backwards compatible to the DCT modes. Unlike
// other mode variations, this one does not require the color transformation
// to be disabled for lossless.
#define JPGFLAG_RESIDUAL_CODING 64
//
// Binary-or this to use an optimized huffman process.
#define JPGFLAG_OPTIMIZE_HUFFMAN 128
//
// Definition of the 'quality' factor of JPEG if finetuning of the quantization
// parameters is not desirable. Defines a quality factor from 1 to 100
// where 100 is best (or lossless for residual or reversible processes)
// and 1 is worst.
#define JPGTAG_IMAGE_QUALITY (JPGTAG_IMAGE_BASE + 0x06)

//
// The residual coding process allows coding with a maximum coding error, i.e.
// a sharp l^\infty error bound. By default, this bound is zero, i.e. coding
// in the residual mode(s) is lossless. If set greater than zero, then
// a non-zero error is permissible. The pixel error may then differ by
// at most by the given bound.
// The same tag also applies to JPEG LS to specify the NEAR parameter.
#define JPGTAG_IMAGE_ERRORBOUND (JPGTAG_IMAGE_BASE + 0x07)

//
// Define the number of resolution levels for pyramidal coding.
// Two special values are available here: If this tag is set to zero, then
// the pyramid consists of a DCT process plus a final lossless (predictive)
// scan allowing lossless coding (just another mechanism). If the tag is set to
// one, then the image is also encoded without loss, but the initial scan is
// a DCT process on an image that is downscaled in each dimension by a factor of
// two.
#define JPGTAG_IMAGE_RESOLUTIONLEVELS (JPGTAG_IMAGE_BASE + 0x08)

//
// Define the type of the color transformation to use to convert from
// the external color transformation (typically RGB) to the internal
// representation. By default, JPEG uses a RGB to YCbCr transformation,
// but other methods can be specified.
#define JPGTAG_IMAGE_COLORTRANSFORMATION (JPGTAG_IMAGE_BASE + 0x09)
// Color transformation methods:
// Use no color transformation, represent data in RGB
#define JPGFLAG_IMAGE_COLORTRANSFORMATION_NONE 0
// Use the standard RGB to YCbCr transformation, which is lossless
// unless residual coding is used to collect the errors.
#define JPGFLAG_IMAGE_COLORTRANSFORMATION_YCBCR 1
// Use the JPEG LS part-2 pseudo RCT transformation with wraparound
// This is only standard conforming for JPEG LS and not for JPEG.
#define JPGFLAG_IMAGE_COLORTRANSFORMATION_LSRCT 2

//
// Identify the image dimensions by a DNL marker instead of putting the
// frame height into the frame header directly.
#define JPGTAG_IMAGE_WRITE_DNL (JPGTAG_IMAGE_BASE + 0x0a)

//
// Define the restart interval. If present, restart markers are written all 'n'
// MCUs, where 'n' is given by this tag. This allows for better error
// resiliance. The algorithm used here is smarter than that used by IJG.
#define JPGTAG_IMAGE_RESTART_INTERVAL (JPGTAG_IMAGE_BASE + 0x0b)

//
// A pointer to a character array defining the subsampling factors of the
// components. Note that these are really the subsampling factors, not the
// dimensions of the components. Thus, only integer subsampling factors
// are supported, and not even all integer subsampling factors are possible
// due to the signaling mechanism specified by JPEG. 444 subsampling has
// all factors set to one, which is also the default. 422 has subsampling
// factors of 1:2:2, etc.
//
// Subsampling factors in X direction. A pointer to a character array.
#define JPGTAG_IMAGE_SUBX (JPGTAG_IMAGE_BASE + 0x0c)
//
// Subsampling factors in Y direction. A pointer to a character array.
#define JPGTAG_IMAGE_SUBY (JPGTAG_IMAGE_BASE + 0x0d)
//
// Tags for defining the progression for the progressive mode.
// Progressive mode expects that this tag is filled out as a pointer to
// another tag list which defines the progression. There should be as many
// tags as there are scans.
#define JPGTAG_IMAGE_SCAN (JPGTAG_IMAGE_BASE + 0x0e)
//
// Quality of the extension layer. This is again a number between 0
// and 100, where 100 indicates lossless.
#define JPGTAG_IMAGE_HDRQUALITY (JPGTAG_IMAGE_BASE + 0x0f)
//
// Parameters defining the coding options: Disable or enable
// the Hadamard transformation.
// Disabling it improves the performance a little bit for lossless coding, though it
// may cause ringing (loss of LSBs) for lossy coding.
// The default of the tag is to enable the Hadamard transformation (on)
#define JPGTAG_IMAGE_ENABLE_HADAMARD (JPGTAG_IMAGE_BASE + 0x10)
//
// Enable noise shaping of the coding residuals. This lowers the coding
// performance but improvides the visual performance of the image.
#define JPGTAG_IMAGE_ENABLE_NOISESHAPING (JPGTAG_IMAGE_BASE + 0x11)
//
// Define the number of hidden DCT bits. These bits of the quantized
// DCT coefficients are encoded in a side-channel not visible to the
// traditional JPEG decoder but they help to improve the accuracy.
#define JPGTAG_IMAGE_HIDDEN_DCTBITS      (JPGTAG_IMAGE_BASE + 0x12)
//
// Parameters defining the HDR mapping procedure. This defines up to four tone mapping curves
// one per component (the code could do more, but the interface is currently limited)
// Each tag is a pointer to 256 UWORDs each defining the HDR level for a given LDR level.
#define JPGTAG_IMAGE_TONEMAPPING0 (JPGTAG_IMAGE_BASE + 0x80)
#define JPGTAG_IMAGE_TONEMAPPING1 (JPGTAG_IMAGE_BASE + 0x81)
#define JPGTAG_IMAGE_TONEMAPPING2 (JPGTAG_IMAGE_BASE + 0x82)
#define JPGTAG_IMAGE_TONEMAPPING3 (JPGTAG_IMAGE_BASE + 0x83)
//

///

/// Tags defining the scan parameters
// These tags are pointed to by JPGTAG_IMAGE_SCAN. There are as many scans
// as there are IMAGE_SCAN tags, for the progressive mode at least.
//
#define JPGTAG_SCAN_BASE (JPGTAG_TAG_USER + 0x300)
//
// Each scan may contain up to four components. The four components
// are defined by the following tags. Default is all components.
#define JPGTAG_SCAN_COMPONENT0 (JPGTAG_SCAN_BASE + 0x01)
#define JPGTAG_SCAN_COMPONENT1 (JPGTAG_SCAN_BASE + 0x02)
#define JPGTAG_SCAN_COMPONENT2 (JPGTAG_SCAN_BASE + 0x03)
#define JPGTAG_SCAN_COMPONENT3 (JPGTAG_SCAN_BASE + 0x04)
//
// Only the chroma components, i.e. anything but zero.
#define JPGTAG_SCAN_COMPONENTS_CHROMA (JPGTAG_SCAN_BASE + 0x05)
//
// Spectral selection: Defines start and end band of the scan,
// both inclusive. Note that the DC band must go into a separate
// scan.
#define JPGTAG_SCAN_SPECTRUM_START (JPGTAG_SCAN_BASE + 0x06)
#define JPGTAG_SCAN_SPECTRUM_STOP  (JPGTAG_SCAN_BASE + 0x07)
//
// Successive approximation: First and last bit of the data
// to be included in the scan.
#define JPGTAG_SCAN_APPROXIMATION_LO (JPGTAG_SCAN_BASE + 0x08)
#define JPGTAG_SCAN_APPROXIMATION_HI (JPGTAG_SCAN_BASE + 0x09)
//
// The point transform for lossless coding modes. This defines the number
// of LSB bits to be shifted out and not represented in the image.
#define JPGTAG_SCAN_POINTTRANSFORM   (JPGTAG_SCAN_BASE + 0x0a)
//
// The component interleaving for JPEG LS scans. Can be single component,
// line interleaved or sample interleaved.
#define JPGTAG_SCAN_LS_INTERLEAVING  (JPGTAG_SCAN_BASE + 0x0b)
//
// The available interleaving types for JPEG_LS
//
// Single component per scan, multiple scans.
#define JPGFLAG_SCAN_LS_INTERLEAVING_NONE 0
//
// Line interleaved, components in a single scan.
#define JPGFLAG_SCAN_LS_INTERLEAVING_LINE 1
//
// Sample interleaved, components in a single scan.
#define JPGFLAG_SCAN_LS_INTERLEAVING_SAMPLE 2
//
// Experimental Vesa proposal.
#define JPGFLAG_SCAN_LS_VESASCAN 3
// Ditto with DCT instead of wavelet.
#define JPGFLAG_SCAN_LS_VESADCTSCAN 4
///

/// Tag definitions for the bitmap IO hook
// Instead of a definition of a data structure, this is just a 
// definition of tag values to be passed to the bitmape IO hook 
// defined by the compressor or decompressor interface function.

#define JPGTAG_BIO_BASE    (JPGTAG_TAG_USER + 0x400)

// The first tag defines the base address where data has to be placed
// or where data has to be read from.
#define JPGTAG_BIO_MEMORY  (JPGTAG_BIO_BASE + 1)

// The next tag item define the width and the height of the bitmap
// *IN PIXELS*
#define JPGTAG_BIO_WIDTH   (JPGTAG_BIO_BASE + 2)
#define JPGTAG_BIO_HEIGHT  (JPGTAG_BIO_BASE + 3)

// We also need to know a so called "modulo value". This is a byte
// offset that must be *ADDED* to the base memory address to go
// from one line down to the next line. In case the video memory is
// organized "upside down", this offset has to be negative.
//
// In a rather typical application, if the total bitmap is 640 pixels
// wide with 8 bits per grey value (non-RGB data), and the memory
// base address increases downwards, then the modulo value would be
// 640 since we have one byte per pixel and 640 pixels, hence 640 bytes
// per row.
#define JPGTAG_BIO_BYTESPERROW (JPGTAG_BIO_BASE + 4)

// Furthermore, we need to know the number of bytes per pixel. 
// Sub-Byte sized pixels are not supported. This tag defaults to one.
// Should be set to two for 16 bits per gun.
#define JPGTAG_BIO_BYTESPERPIXEL (JPGTAG_BIO_BASE + 5)

// Furthermore, the type of data to be put into the bitmap.
// Currently, we support only JPGFLAG_BYTE,JPGFLAG_UBYTE 
// and JPGFLAG_WORD,JPGFLAG_UWORD,JPGFLAG_SWORD,JPGFLAG_SUWORD 
// and JPGFLAG_FIXPOINT
#define JPGTAG_BIO_PIXELTYPE (JPGTAG_BIO_BASE + 6)

// The next four tag items describe in the so-called "reference grid"
// coordinates of the jpeg2000 system which rectangle of the image you
// want to retrieve, or the jk2lib wants to retrieve from you.
// Coordinates are min/max values with min and max both inclusive.
#define JPGTAG_BIO_MINX   (JPGTAG_BIO_BASE + 16)
#define JPGTAG_BIO_MINY   (JPGTAG_BIO_BASE + 17)
#define JPGTAG_BIO_MAXX   (JPGTAG_BIO_BASE + 18)
#define JPGTAG_BIO_MAXY   (JPGTAG_BIO_BASE + 19)

// The origin of the bitmap is always anchored at the origin
// of the reference grid.
// The coordinates given above is always given in canvas coordinates
// that means, without downsampling, i.e. the library will divide these
// coordinates by the subsampling values to get the pixel offsets.


// The requested rectangle in pixel instead of canvas coordinates.
// The coordinates you find here are already divided by subsampling
// factors (as applicable) and the thumbnail scaling exponent.
#define JPGTAG_BIO_PIXEL_MINX (JPGTAG_BIO_BASE + 24)
#define JPGTAG_BIO_PIXEL_MINY (JPGTAG_BIO_BASE + 25)
#define JPGTAG_BIO_PIXEL_MAXX (JPGTAG_BIO_BASE + 26)
#define JPGTAG_BIO_PIXEL_MAXY (JPGTAG_BIO_BASE + 27)

// The canvas origin in pixel coordinates.
#define JPGTAG_BIO_PIXEL_XORG (JPGTAG_BIO_BASE + 28)
#define JPGTAG_BIO_PIXEL_YORG (JPGTAG_BIO_BASE + 29)

// We furthermore need to know which component of the image you want
// to receive, or which component of the image is requested.
// For standard J2K images, this would be 0 for Y, 1 for U and 2 for V
// or 0 for R, 1 for G and 2 for B.
// For JPX arbitrary multi-component transformation, this index is
// the generated component index rather than the canvas component
// index.
#define JPGTAG_BIO_COMPONENT (JPGTAG_BIO_BASE + 32)

// We also give a tag item whether this request was for a ROI image or
// not. This is set to TRUE if it is for a ROI, false otherwise.
// For the case we request a ROI, the BIO_COMPONENT tag is also
// available, but might have a different meaning, depending on the
// BIO_COLORTRAFO tag below.
#define JPGTAG_BIO_ROI       (JPGTAG_BIO_BASE + 33)

// In case this is a ROI, we also give the decomposition level of
// this ROI mask. 0 is topmost, means the ROI will be added to all
// resolution levels and will be scaled downwards. You may also add
// ROIs to higher decomposition levels and hence lower resolutions
// as to emphrasis only lower frequencies with a ROI.
//
// OBSOLETE, no longer supported as of 3.0
//#define JPGTAG_BIO_DECOMPOSITION (JPGTAG_BIO_BASE + 34)

// The following tag is provided for ROIs only: If the value of this
// tag is zero, then the component information above is really
// the same component you passed in. However, if it is not, the
// library will have performed a color transformation on your
// image, for example a RGB->YCbCr conversion. In this case, the
// component information will relate to the transformed component
// index (Y, Cb or Cr) as above instead of R,G,B. In case your ROI
// is not component specific, you need not to check for this
// and the component above at all, otherwise feel prepared that
// your ROI won't do exactly what you want if you want the R 
// component for more important than G or B. This simply won't go
// if a color transformation is turned on, obviously.
//
// OBSOLETE, no longer supported as of 3.0. Replaced by the tag below.
//#define JPGTAG_BIO_COLORTRAFO (JPGTAG_BIO_BASE + 35)

// This tag is provided for ROIs only. It defines the number of
// components the ROI is valid for. E.g., for color transformed components,
// this could be three, as the ROI is valid for all three components at once.
// BIO_COMPONENT is the first component affected, BIO_COMPONENT + BIO_RANGE -1
// the last one.
#define JPGTAG_BIO_RANGE (JPGTAG_BIO_BASE + 36) 

// The next item is purely for your own use. Since you have to ensure
// that the requested rectangle stays available as long as the j2lib
// scans the image, this item can be used to hold a "lock" on the
// rectangle of any kind. For example, it could be a pointer to 
// temporary memory you had to allocate for passing it over into
// the library, such as to be able to release it when the library calls
// the "release" vector.
#define JPGTAG_BIO_USERDATA (JPGTAG_BIO_BASE + 64)

// The next item is only available on decoding and can be found
// on the tag list that is passed in on a "bitmap hook release".
// It is set to FALSE by the hook, but you can enforce it to TRUE.
// In case you do, the library will call your hook again, with the
// same parameters. This is sometimes required if you want to
// map one component into several different destinations.
// Note that in case a color transformation is enabled, not only
// *this* request will be repeated, but also the request for the
// related components. Hence, if you specify "come again" for
// component 1 of a RGB->YCbCr color transformed image, the
// bitmap hook will request components 0 to 2 again. As always,
// you may deliver NULL as BIO_MEMORY if you do not need the
// other components.
// OBSOLETE in 3.xx. Use automatic upsampling if required.
//#define JPGTAG_BIO_COME_AGAIN (JPGTAG_BIO_BASE + 80)

// The next one defines what the library wants from you. There
// are currently two possible actions: A "request" action where
// get a rectangle passed in and have to supply a memory chunk
// where the library may fetch data from, and a "release" action
// where the corresponding rectangle will be released. Each 
// request is paired by one release, except for the situation where
// the library runs in an error condition in the middle.
//
// On request, your hook gets a tag list passed in. This tag list
// contains for once the parameters of the request, for example
// the rectangle coordinates with the BIO_MINX,... calls, but
// it also contains the tag items defined above describing the
// bitmap layout. These tag-items are pre-defined from the same
// tag items you passed in when calling the library, but you are
// welcome here to modify (or replace them partially!) here such
// as to tell the library that your requirements changed because,
// for example, a part of the image wasn't available at the time
// you called the library. 
// Hence, you *may* pass the tags above as default values when
// making the initial call to the library in which case you'll find
// your own values back here again. In the easiest situation, you
// won't need to touch them.
// In somewhat more tricky applications where the layout of the
// image is not yet known, pass some reasonable defaults to the library,
// or don't pass any values on the initial call, and then correct
// or setup the tags if the "request" passes by. 
//
// The second possible command is the "release" operation. It is
// for your own purpose and should be used by your application as
// indicator that the libjpeg is "done" with the requested part of
// the image. This could be used to release temporary memory you
// could have allocated, or to unlock the image from memory, or
// to advance the paper position on a scanner, or...
#define JPGTAG_BIO_ACTION (JPGTAG_BIO_BASE + 65)

#define JPGFLAG_BIO_REQUEST 'R'
#define JPGFLAG_BIO_RELEASE 'r'

// The next tag items are not used for the call back
// hooks, but *MUST* be present when you call the library as it will
// otherwise not be able to get image data from your application:
#define JPGTAG_BIH_BASE      (JPGTAG_TAG_USER + 0x500)

// Define the hook function to be called. This tag item
// takes a pointer to a hook structure as tag data.
// See hooks.hpp for details.
// This here is *mandatory*
#define JPGTAG_BIH_HOOK      (JPGTAG_BIH_BASE + 0x01)
///
/// Tag definitions for the file hook
// Instead of a definition of a data structure, this is just a 
// definition of tag values to be passed to the file IO hook 
// defined by the compressor or decompressor interface function.

#define JPGTAG_FIO_BASE    (JPGTAG_TAG_USER + 0x100)

// The stream is passed by the following tag item. What a stream is
// is the matter of the client. For the library, this is just a "dummy"
// that is not interpreted at all.
#define JPGTAG_FIO_HANDLE  (JPGTAG_FIO_BASE + 1)

// The following tag item submits a pointer to the client. It contains
// the buffer into which or from which data has to be written or has to
// be read from. No interpretation of this data should be made. It should
// be considered as an array of UBYTE.
#define JPGTAG_FIO_BUFFER  (JPGTAG_FIO_BASE + 2)

// The following tag item defines the number of bytes to be read or written
// to. Defaults to one.
#define JPGTAG_FIO_SIZE    (JPGTAG_FIO_BASE + 3)

// The following tag item describes the IO operation to be performed.
// Currently, read and write requests are available. 
// As soon as the FDIS file format is implemented, seek operations might
// be useful as well:
#define JPGTAG_FIO_ACTION  (JPGTAG_FIO_BASE + 4)

// What we may use as ACTIONS. This may change.
// Note that the library does not read AND write simulatenously,
// hence there's no problem with proper flushing here.
#define JPGFLAG_ACTION_READ  'R'  // Read data
#define JPGFLAG_ACTION_WRITE 'W'  // Write data
#define JPGFLAG_ACTION_SEEK  'S'  // Skip data
#define JPGFLAG_ACTION_QUERY 'Q'  // Returns status information on failure

// Userdata for the file hook.
#define JPGTAG_FIO_USERDATA (JPGTAG_FIO_BASE + 7)

#define JPGTAG_FIO_SEEKMODE (JPGTAG_FIO_BASE + 5)
// the seek mode is one of the following:
#define JPGFLAG_OFFSET_CURRENT    0  // relative to the current FP
#define JPGFLAG_OFFSET_BEGINNING -1  // relative to the start of the file
#define JPGFLAG_OFFSET_END        1  // relative to the end of the file
// This passes the offset, i.e. how many bytes should be skipped
// or seek'd.
#define JPGTAG_FIO_OFFSET   (JPGTAG_FIO_BASE + 6)
///
/// Parameters for passing the various hooks to the library
#define JPGTAG_HOOK_BASE       (JPGTAG_TAG_USER + 0xb00)

// The following defines a pointer to the I/O hook.
// This hook is used by the library to write encoded data
// out or to read encoded data for decoding. It requires
// a pointer to a hook structure, see hooks.hpp.
#define JPGTAG_HOOK_IOHOOK     (JPGTAG_HOOK_BASE + 0x01)

// Furthermore, we also require a "stream". What precisely
// a stream is depends really on your application. All
// the library has to know about streams is that they are
// passed as argument to the IoHook for reading or
// writing.
#define JPGTAG_HOOK_IOSTREAM   (JPGTAG_HOOK_BASE + 0x02)

// Defines a buffer size for internal buffering of the
// data before your hook is called to write a "bulk" of
// data out. Hence, no second level Unix style IO function
// would be required here for buffering.
// Buffer size defaults to 2K = 2048 bytes.
#define JPGTAG_HOOK_BUFFERSIZE (JPGTAG_HOOK_BASE + 0x03)

// Optionally, a user provided buffer that contains the
// bytes to be read, or will be filled with the bytes
// written. If NULL, the hook will allocate a buffer for
// you and pass the buffer into the hook. Otherwise,
// this buffer is passed into the hook. The hook may
// modify the input tags and by that change the buffer
// each time, regardless of whether this is the user's
// buffer, or the library buffer.
// Default is NULL, i.e. the library allocates the buffer.
// Note that if you provide a buffer, it must be the size
// of the above.
#define JPGTAG_HOOK_BUFFER    (JPGTAG_HOOK_BASE + 0x04)

// Only for GetInformation(): This tag returns the number of
// bytes that are still waiting in the input buffer of the
// library and that haven't been read off so far. This 
// information might prove useful re-insert the un-read
// data back into another call if the library reads parts
// of an alien file format.
#define JPGTAG_HOOK_REMAININGBYTES (JPGTAG_HOOK_BASE + 0x08)
///

/// Memory Hook related
#define JPGTAG_MEMORY_BASE (JPGTAG_TAG_USER + 0x2000)
//
// The size of the memory to be requested or released
// This is always the first tag.
#define JPGTAG_MIO_SIZE (JPGTAG_MEMORY_BASE + 0x01)
//
// The type of the memory to be requested. There are
// currently no types defined.
// This is always the second tag on a request, and not
// available on release.
#define JPGTAG_MIO_TYPE (JPGTAG_MEMORY_BASE + 0x02)
//
// The pointer to the memory to be released. This is
// not available on allocation, but it is always the
// second tag on release.
#define JPGTAG_MIO_MEMORY (JPGTAG_MEMORY_BASE + 0x03)
//
// A user-specific pointer you can do whatever you like
// with: This is allocation specific.
#define JPGTAG_MIO_ALLOC_USERDATA (JPGTAG_MEMORY_BASE + 0x10)
// and this is release specific
#define JPGTAG_MIO_RELEASE_USERDATA (JPGTAG_MEMORY_BASE + 0x11)
//
// The pointer to the memory hook to be given on the main tag list:
// This is for allocation...
#define JPGTAG_MIO_ALLOC_HOOK   (JPGTAG_MEMORY_BASE + 0x20)
// and this for release
#define JPGTAG_MIO_RELEASE_HOOK (JPGTAG_MEMORY_BASE + 0x21)
//
// If you set the following tag to FALSE, the library will not
// try to remember the size of the allocated memory block but
// requires the client hook to ignore the size. If this is TRUE,
// the library will pass out the size of the allocated memory
// block on the memory release. This might cause a very mild
// overhead for some allocations.
#define JPGTAG_MIO_KEEPSIZE     (JPGTAG_MEMORY_BASE + 0x30)
//
///

/// Parameters for the decoder
#define JPGTAG_DECODER_BASE            (JPGTAG_TAG_USER + 0xf00)

// The following tags are read by the decoder::RequestRectangleForDisplay
// They define the rectangle, components and layer coordinates of the
// data that shall be kept in the decoder.

// For first, the rectangle definition
// This is by default all of the canvas
#define JPGTAG_DECODER_MINX            (JPGTAG_DECODER_BASE + 0x01)
#define JPGTAG_DECODER_MINY            (JPGTAG_DECODER_BASE + 0x02)
#define JPGTAG_DECODER_MAXX            (JPGTAG_DECODER_BASE + 0x03)
#define JPGTAG_DECODER_MAXY            (JPGTAG_DECODER_BASE + 0x04)

// If the following tag is set, the addressed components are rather
// color functions of the color manager, i.e. MINCOMPONENT and MAXCOMPONENT
// are color functions to be requested. The default for this tag is
// to use color function addressing whenever the color manager is enabled,
// and to use generated components otherwise.
// Currently ignored, though...
#define JPGTAG_DECODER_USE_COLORS      (JPGTAG_DECODER_BASE + 0x15)

// Then the minimal and maximal component of the request
// Currently unusued.
#define JPGTAG_DECODER_MINCOMPONENT    (JPGTAG_DECODER_BASE + 0x05)
#define JPGTAG_DECODER_MAXCOMPONENT    (JPGTAG_DECODER_BASE + 0x06)
//
// This tag specifies the upsampling/downsampling mode for encoding
// or decoding. This will also turn all given coordinates
// in the bitmap hook into canvas rather than subsampled canvas
// coordinates. This is for DisplayRectangle() only, though, not
// for encoding.
// Currently ignored, do not use right now.
#define JPGTAG_DECODER_UPSAMPLE        (JPGTAG_DECODER_BASE + 0x0b)
//
// Force reconstructing with the reversible DCT even though the headers
// say otherwise (mostly for testing)
#define JPGTAG_DECODER_FORCEINTEGERDCT (JPGTAG_DECODER_BASE + 0x10)
//
// Force reconstructing with the fixpoint (non-reversibe) DCT even though
// the headers say otherwise (again only for testing)
#define JPGTAG_DECODER_FORCEFIXPOINTDCT (JPGTAG_DECODER_BASE + 0x11)
//
// Parsing flags - these define when the decoder (or encoder) stop, i.e.
// after which syntax elements the call returns. If it does, the code needs
// to re-enter the image after reading it until it is complete.
#define JPGTAG_DECODER_STOP             (JPGTAG_DECODER_BASE + 0x20)
//
// Possible stop flags
//
// Stop after individual MCUs
#define JPGFLAG_DECODER_STOP_MCU    0x01
// Stop after MCU rows
#define JPGFLAG_DECODER_STOP_ROW    0x02
// Stop after scans
#define JPGFLAG_DECODER_STOP_SCAN   0x04
// Stop after frames 
#define JPGFLAG_DECODER_STOP_FRAME  0x08
// Stop after the header
#define JPGFLAG_DECODER_STOP_IMAGE  0x10
///

/// Parameters for the encoder
//
#define JPGTAG_ENCODER_BASE            (JPGTAG_TAG_USER + 0xf80)
//
// Parsing flags - these define when the decoder (or encoder) stop, i.e.
// after which syntax elements the call returns. If it does, the code needs
// to re-enter the image after reading it until it is complete.
#define JPGTAG_ENCODER_STOP JPGTAG_DECODER_STOP
//
// Possible stop flags
//
// Stop after individual MCUs
#define JPGFLAG_ENCODER_STOP_MCU   JPGFLAG_DECODER_STOP_MCU
// Stop after MCU rows
#define JPGFLAG_ENCODER_STOP_ROW   JPGFLAG_DECODER_STOP_ROW
// Stop after scans
#define JPGFLAG_ENCODER_STOP_SCAN  JPGFLAG_DECODER_STOP_SCAN
// Stop after frames 
#define JPGFLAG_ENCODER_STOP_FRAME JPGFLAG_DECODER_STOP_FRAME
// Stop after the header
#define JPGFLAG_ENCODER_STOP_IMAGE JPGFLAG_DECODER_STOP_IMAGE
//
// Set on encoding when the image is completely provided.
#define JPGTAG_ENCODER_IMAGE_COMPLETE (JPGTAG_ENCODER_BASE + 0x01)
//
// Define this to automatically loop in provide image when the image is not
// yet complete
#define JPGTAG_ENCODER_LOOP_ON_INCOMPLETE (JPGTAG_ENCODER_BASE + 0x02)
///

/// Exception related hooks
// The following tags are related to the hooks that are called
// on exceptions and warnings.
#define JPGTAG_EXCEPTION_BASE  (JPGTAG_TAG_USER + 0x2100)
//
// Defines the error code of the exception on the user
// called hook
#define JPGTAG_EXC_ERROR       (JPGTAG_EXCEPTION_BASE + 0x01)
//
// Defines the class within this exception was caught. Typically,
// an application need not to care about this
#define JPGTAG_EXC_CLASS       (JPGTAG_EXCEPTION_BASE + 0x02)
//
// Defines the line number of the exception within the source
// file. This is typically not interesting for clients.
#define JPGTAG_EXC_LINE        (JPGTAG_EXCEPTION_BASE + 0x03)
//
// Defines the name of the source file the exception was caught
// within. Like the above, no need to care about.
#define JPGTAG_EXC_SOURCE      (JPGTAG_EXCEPTION_BASE + 0x04)
//
// Defines an error description as an ASCII string. Is NULL for
// internal errors.
#define JPGTAG_EXC_DESCRIPTION (JPGTAG_EXCEPTION_BASE + 0x05)
//
// Defines the hook to be called in case an exception is detected.
// Default is not to call any hook but to abort execution and
// to return with an error code
#define JPGTAG_EXC_EXCEPTION_HOOK (JPGTAG_EXCEPTION_BASE + 0x10)
// Defines a similar hook for warnings
#define JPGTAG_EXC_WARNING_HOOK   (JPGTAG_EXCEPTION_BASE + 0x11)
//
// Defines user data to be supplied on exceptions
#define JPGTAG_EXC_EXCEPTION_USERDATA (JPGTAG_EXCEPTION_BASE + 0x20)
// Defines user data to be supplied on warnings
#define JPGTAG_EXC_WARNING_USERDATA (JPGTAG_EXCEPTION_BASE + 0x21)
//
// If the following tag is set on construction of the library, the
// library will try to lower the number of warnings by suppressing
// multiple warnings of the same origin.
#define JPGTAG_EXC_SUPPRESS_IDENTICAL (JPGTAG_EXCEPTION_BASE + 0x30)
///
/// Application Program Base
// If your application needs to use custom tags that are passed to the
// libjpeg, you have to make sure that the libjpeg does not use and will
// never use that tag id. Therefore, use tag ids that are larger than the
// following tag. These are guaranteed to be never used by the libjpeg.
#define JPGTAG_APP_BASE (JPGTAG_TAG_USER + 0x10000)
///

/// Exception types
// Exception types defined here.
// These are the exceptions you get by either the exception hook or the
// warning hook

#define JPGERR_INVALID_PARAMETER   -1024
// A parameter for a function was out of range.

#define JPGERR_UNEXPECTED_EOF      -1025
// Stream run out of data.

#define JPGERR_UNEXPECTED_EOB      -1026
// A code block run out of data.

#define JPGERR_STREAM_EMPTY        -1027
// Tried to perform an unputc or or an unget on an empty stream.

#define JPGERR_OVERFLOW_PARAMETER  -1028
// some parameter run out of range

#define JPGERR_NOT_AVAILABLE       -1029
// the requested operation does not apply

#define JPGERR_OBJECT_EXISTS       -1030
// tried to create an already existing obj

#define JPGERR_OBJECT_DOESNT_EXIST -1031
// tried to access a non-existing obj

#define JPGERR_MISSING_PARAMETER   -1032
// a non-optional parameter was left out

#define JPGERR_BAD_STREAM          -1033
// forgot to delay a 0xff. How that???

#define JPGERR_OPERATION_UNIMPLEMENTED -1034
#define JPGERR_NOT_IMPLEMENTED     -1034
// The requested operation is not available. This is an
// internal error.

#define JPGERR_PHASE_ERROR         -1035
// an item computed on a former pass does not coincide with the same 
// item on a latter pass. Internal error.

#define JPGERR_NO_JPG              -1036
// the stream passed in is no valid jpeg stream.

#define JPGERR_DOUBLE_MARKER       -1037
// a unique marker turned up more than once.
// The input stream is most likely corrupt.

#define JPGERR_MALFORMED_STREAM    -1038
// a misplaced marker segment showed up

#define JPGERR_NOT_IN_PROFILE      -1040
// The specified parameters are valid, but are not supported by
// the selected profile. Either use a higher profile, or use
// simpler parameters (encoder only).

#define JPGERR_THREAD_ABORTED      -1041
// The worker thread that was currently active had to terminate
// unexpectedly. This is an internal error that should not happen.

#define JPGERR_INVALID_HUFFMAN     -1042
// The encoder tried to emit a symbol for which no Huffman code
// was defined. This happens if the standard Huffman table is used
// for an alphabet for which it was not defined. The reaction
// to this exception should be to create a custom huffman table
// instead.

#define JPGERR_OUT_OF_MEMORY       -2048
// The library run out of memory.


// Errors below this value (or "above" if you'd look at the absolute value)
// are user-supplied errors of your hook function. The library
// will never try to throw one of these.
#define JPGERR_USER_ERROR         -8192
//
///

///
#endif
