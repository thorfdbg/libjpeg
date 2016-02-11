/*
**
** This class is a helper to keep a linked list of one-D lines
** for buffering purposes.
**
** $Id: line.hpp,v 1.8 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef TOOLS_LINE_HPP
#define TOOLS_LINE_HPP

/// Includes
#include "tools/environment.hpp"
///

/// struct Line
// A single line containing the source data for upsampling.
struct Line : public JObject {
  //
  // The data. Width is not kept here.
  LONG             *m_pData;
  //
  // Pointer to the next line.
  struct Line      *m_pNext;
  //
#if CHECK_LEVEL > 0
  void             *m_pOwner;
#endif
  //
  Line(void)
    : m_pData(NULL), m_pNext(NULL)
  { }
};
///

///
#endif
