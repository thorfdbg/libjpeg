/*
**
** This file defines a class that implements the component upsampling
** procedure.
**
** $Id: upsampler.hpp,v 1.10 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef UPSAMPLING_UPSAMPLER_HPP
#define UPSAMPLING_UPSAMPLER_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "upsampling/upsamplerbase.hpp"
///

/// Class Upsampler
// This class performs the upsampling process.
template<int sx,int xy>
class Upsampler : public UpsamplerBase {
  //
  //
public:
  Upsampler(class Environ *env,ULONG width,ULONG height);
  //
  virtual ~Upsampler(void);
  //
  // The actual upsampling process.
  virtual void UpsampleRegion(const RectAngle<LONG> &r,LONG *buffer) const;
  //
};
///

///
#endif

