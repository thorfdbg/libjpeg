/*
** This helper class keeps and updates the checksum over the legacy
** JPEG stream.
**
** $Id: checksum.hpp,v 1.7 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef TOOLS_CHECKSUM_HPP
#define TOOLS_CHECKSUM_HPP

/// Includes
#include "interface/types.hpp"
#include "tools/environment.hpp"
///

/// Defines
//#define TESTING
#ifdef TESTING
#include "std/stdio.hpp"
#endif
/// 

/// class Checksum
// This helper class keeps and updates the checksum over the legacy
// JPEG stream.
class Checksum : public JObject {
  //
  // Consists of two bytes.
  UBYTE m_ucCount1;
  UBYTE m_ucCount2;
  //
#ifdef TESTING
  FILE *tmpout;
  int line;
#endif
  //
public:
  Checksum(void)
  : m_ucCount1(0), m_ucCount2(0)
  { 
#ifdef TESTING
    tmpout = fopen("/tmp/chksum","w"); 
    line   = 0;
#endif
  }
  //
  ~Checksum(void)
  {
#ifdef TESTING
    if (tmpout)
      fclose(tmpout);
    tmpout = NULL;
#endif
  }
  //
  // Return the checksum so far.
  ULONG ValueOf(void) const
  {
    return m_ucCount1 | (m_ucCount2 << 8);
  }
  //
  // Update the checksum for a data block.
  void Update(const UBYTE *b,ULONG size)
  {
    UWORD sum;
    //
    while(size) {
      sum  = m_ucCount1;
#ifdef TESTING
      if (tmpout) {
	fprintf(tmpout,"%10d\t%02x\n",line++,*b);
      }
#endif
      sum += *b++;
      sum += (sum + 1) >> 8; // sum modulo 255
      m_ucCount1 = UBYTE(sum);
      assert(m_ucCount1 != 0xff);
      
      sum  = m_ucCount2;
      sum += m_ucCount1;
      sum += (sum + 1) >> 8;
      m_ucCount2 = UBYTE(sum);
      assert(m_ucCount2 != 0xff);
      
      size--;
    }
  }
  //
  // Update the checksum for a single byte.
  void Update(UBYTE b)
  {
    UWORD sum;
    //
    sum  = m_ucCount1;
#ifdef TESTING
    if (tmpout) {
      fprintf(tmpout,"%10d\t%02x\n",line++,b);
    }
#endif
    sum += b;
    sum += (sum + 1) >> 8; // sum modulo 255
    m_ucCount1 = UBYTE(sum);
    assert(m_ucCount1 != 0xff);
      
    sum  = m_ucCount2;
    sum += m_ucCount1;
    sum += (sum + 1) >> 8;
    m_ucCount2 = UBYTE(sum);
    assert(m_ucCount2 != 0xff);
  }
};
///

///
#endif
