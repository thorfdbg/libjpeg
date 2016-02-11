/*
** This class is responsible for parsing the AC specific part of the
** DHT marker and generating the corresponding decoder classes.
**
** $Id: arithmetictemplate.hpp,v 1.7 2014/09/30 08:33:16 thor Exp $
**
*/

#ifndef CODING_ARITHMETICTEMPLATE_HPP
#define CODING_ARITHMETICTEMPLATE_HPP

/// Includes
#include "coding/decodertemplate.hpp"
///

/// Forwards
///

/// class ArithmeticTemplate
class ArithmeticTemplate : public DecoderTemplate {
  //
public:
  ArithmeticTemplate(class Environ *env);
  //
  ~ArithmeticTemplate(void);
  //
  // Parse the AC specific part of the DHT table.
  virtual void ParseMarker(class ByteStream *io);
};
///

///
#endif

  
