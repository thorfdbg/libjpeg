/*
** This box keeps the file type and compatible file types for JPEG XT.
** It is basically a profile information.
**
** $Id: filetypebox.hpp,v 1.5 2015/03/11 16:02:38 thor Exp $
**
*/

#ifndef BOXES_FILETYPEBOX_HPP
#define BOXES_FILETYPEBOX_HPP

/// Includes
#include "boxes/box.hpp"
///

/// FileTypeBox
// This box keeps the file type and compatible file types for JPEG XT.
// It is basically a profile information.
class FileTypeBox : public Box {
  //
  // The brand type. This must be jpxt
  ULONG m_ulBrand;
  //
  // Minor version.
  ULONG m_ulMinor;
  //
  // Size of the compatibility list.
  ULONG m_ulNumCompats;
  //
  // The compatibility list.
  ULONG *m_pulCompatible; 
  //
  // Second level parsing stage: This is called from the first level
  // parser as soon as the data is complete. Must be implemented
  // by the concrete box. Returns true in case the contents is
  // parsed and the stream can go away.
  virtual bool ParseBoxContent(class ByteStream *stream,UQUAD boxsize);
  //
  // Second level creation stage: Write the box content into a temporary stream
  // from which the application markers can be created.
  // Returns whether the box content is already complete and the stream
  // can go away.
  virtual bool CreateBoxContent(class MemoryStream *target);
  //
public:
  enum {
    Type          = MAKE_ID('f','t','y','p'), // box type
    XT_Brand      = MAKE_ID('j','p','x','t'),
    XT_IDR        = MAKE_ID('i','r','f','p'), // intermediate range coding following 18477-6
    XT_HDR_A      = MAKE_ID('x','r','d','d'), // Profile A of 18477-7
    XT_HDR_B      = MAKE_ID('x','r','x','d'), // Profile B of 18477-7
    XT_HDR_C      = MAKE_ID('x','r','a','d'), // Profile C of 18477-7
    XT_HDR_D      = MAKE_ID('x','r','r','f'), // Profile D of 18477-7
    XT_LS         = MAKE_ID('l','s','f','p'), // lossless range coding following 18477-8
    XT_ALPHA_FULL = MAKE_ID('a','c','f','p'), // alpha coding full profile, 18477-9
    XT_ALPHA_BASE = MAKE_ID('a','c','b','p')  // alpha coding base profile, 18477-9
  };
  //
  FileTypeBox(class Environ *env,class Box *&boxlist)
    : Box(env,boxlist,Type), m_ulBrand(XT_Brand), m_ulMinor(0),
      m_ulNumCompats(0), m_pulCompatible(NULL)
  { }
  //
  virtual ~FileTypeBox(void);
  //
  // Add an entry to the compatibility list.
  void addCompatibility(ULONG compat);
  //
  // Check whether this file is compatible to the listed profile ID.
  // It is always understood to be compatibile to the brand, otherwise reading
  // would fail.
  bool isCompatbileTo(ULONG compat) const;
  //
};
///

///
#endif
