/*
** This box keeps the file type and compatible file types for JPEG XT.
** It is basically a profile information.
**
** $Id: filetypebox.cpp,v 1.3 2014/09/30 08:33:14 thor Exp $
**
*/

/// Includes
#include "boxes/box.hpp"
#include "boxes/filetypebox.hpp"
#include "io/bytestream.hpp"
#include "io/memorystream.hpp"
///

/// FileTypeBox::~FileTypeBox
// Destroy a file type box
FileTypeBox::~FileTypeBox(void)
{
  if (m_pulCompatible) {
    m_pEnviron->FreeMem(m_pulCompatible,sizeof(ULONG) * m_ulNumCompats);
  }
}
///

/// FileTypeBox::ParseBoxContent
// Second level parsing stage: This is called from the first level
// parser as soon as the data is complete. Must be implemented
// by the concrete box. Returns true in case the contents is
// parsed and the stream can go away.
bool FileTypeBox::ParseBoxContent(class ByteStream *stream,UQUAD boxsize)
{
  LONG lo,hi;
  ULONG *cmp,cnt;
  //
  if (boxsize < 4 + 4) // at least brand and minor version must be there.
    JPG_THROW(MALFORMED_STREAM,"FileTypeBox::ParseBoxContent",
	      "Malformed JPEG stream - file type box is too short to contain brand and minor version");
  //
  // Also, refuse too long boxes.
  if (boxsize > MAX_LONG / sizeof(LONG))
    JPG_THROW(MALFORMED_STREAM,"FileTypeBox::ParseBoxContent",
	      "Malformed JPEG stream - file type box is too long or length is invalid");
  //
  assert(m_pulCompatible == NULL);
  //
  hi = stream->GetWord();
  lo = stream->GetWord();
  //
  // Check whether the brand is ok.
  if (((hi << 16) | lo) != XT_Brand)
    JPG_THROW(MALFORMED_STREAM,"FileTypeBox::ParseBoxContent",
	      "Malformed JPEG stream - file is not compatible to JPEG XT and cannot be read by this software");
  //
  // Ignore the minor version.
  stream->GetWord();
  stream->GetWord();
  //
  // Remove bytes read so far.
  boxsize -= 4 + 4;
  //
  // Now check the number of entries.
  if (boxsize & 3)
    JPG_THROW(MALFORMED_STREAM,"FileTypeBox::ParseBoxContent",
	      "Malformed JPEG stream - number of compatibilities is corrupted, "
	      "box size is not divisible by entry size");
  //
  cnt = boxsize >> 2;
  m_ulNumCompats  = cnt;
  m_pulCompatible = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * cnt);
  cmp = m_pulCompatible;
  while(cnt) {
    hi     = stream->GetWord();
    lo     = stream->GetWord();
    *cmp++ = (hi << 16) | lo;
    cnt--;
  }

  return true;
}
///

/// FileTypeBox::CreateBoxContent
// Second level creation stage: Write the box content into a temporary stream
// from which the application markers can be created.
// Returns whether the box content is already complete and the stream
// can go away.
bool FileTypeBox::CreateBoxContent(class MemoryStream *target)
{
  const ULONG *cmp = m_pulCompatible;
  ULONG cnt = m_ulNumCompats;

  target->PutWord(m_ulBrand >> 16);
  target->PutWord(m_ulBrand & 0xffff);
  target->PutWord(m_ulMinor >> 16);
  target->PutWord(m_ulMinor & 0xffff);

  while(cnt) {
    target->PutWord(*cmp >> 16);
    target->PutWord(*cmp & 0xffff);
    cmp++;
    cnt--;
  }

  return true;
}
///

/// FileTypeBox::addCompatibility 
// Add an entry to the compatibility list.
void FileTypeBox::addCompatibility(ULONG compat)
{
  ULONG newcnt = m_ulNumCompats + 1;
  ULONG *p;

  if (newcnt <= m_ulNumCompats)
    JPG_THROW(OVERFLOW_PARAMETER,"FileTypeBox::addCompatibility",
	      "too many compatible brands specified, cannot add another");

  p = (ULONG *)m_pEnviron->AllocMem(sizeof(ULONG) * newcnt);

  if (m_pulCompatible && m_ulNumCompats > 0) {
    memcpy(p,m_pulCompatible,m_ulNumCompats * sizeof(ULONG));
    m_pEnviron->FreeMem(m_pulCompatible,m_ulNumCompats * sizeof(ULONG));
    m_pulCompatible = NULL;
  }

  p[m_ulNumCompats] = compat;
  m_pulCompatible   = p;
  m_ulNumCompats    = newcnt;
}
///

/// FileTypeBox::isCompatbileTo
// Check whether this file is compatible to the listed profile ID.
// It is always understood to be compatibile to the brand, otherwise reading
// would fail.
bool FileTypeBox::isCompatbileTo(ULONG compat) const
{
  const ULONG *p = m_pulCompatible;
  ULONG cnt      = m_ulNumCompats;

  if (p) {
    while(cnt) {
      if (*p == compat)
	return true;
      p++;
      cnt--;
    }
  }

  return false;
}
///
