/*
** This class contains and maintains the AC conditioning
** parameters.
**
** $Id: actemplate.hpp,v 1.8 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CODING_ACTEMPLATE_HPP
#define CODING_ACTEMPLATE_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Forwards
class ByteStream;
///

/// ACTemplate
// This class contains and maintains the AC conditioning
// parameters.
#if ACCUSOFT_CODE
class ACTemplate : public JKeeper {
  //
  // The lower threshold, also known as 'L' parameter in the specs.
  UBYTE m_ucLower;
  //
  // The upper threshold parameter, also known as 'U' parameter.
  UBYTE m_ucUpper;
  //
  // The block index that discriminates between lower and upper
  // block indices for AC coding.
  UBYTE m_ucBlockEnd;
  //
public:
  ACTemplate(class Environ *env);
  //
  ~ACTemplate(void);
  //
  // Parse off DC conditioning parameters.
  void ParseDCMarker(class ByteStream *io);
  //
  // Parse off an AC conditioning parameter.
  void ParseACMarker(class ByteStream *io);
  //
  // Just install the defaults found in the standard.
  void InitDefaults(void);
  //
  // Return the largest block index that still counts
  // as a lower index for AC coding. This is the kx parameter
  UBYTE BandDiscriminatorOf(void) const
  {
    return m_ucBlockEnd;
  }
  //
  // Return the L parameter.
  UBYTE LowerThresholdOf(void) const
  {
    return m_ucLower;
  }
  //
  // Return the U parameter.
  UBYTE UpperThresholdOf(void) const
  {
    return m_ucUpper;
  }
};
#endif
///

///
#endif
