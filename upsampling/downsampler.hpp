/*
** The actual downsampling implementation.
**
** $Id: downsampler.hpp,v 1.8 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef UPSAMPLING_DOWNSAMPLER_HPP
#define UPSAMPLING_DOWNSAMPLER_HPP

/// Includes
#include "tools/environment.hpp"
#include "tools/rectangle.hpp"
#include "upsampling/downsamplerbase.hpp"
///

/// Class Downsampler
// This class implements the downsampling process
// with several downsampling factors.
template<int sx,int sy>
class Downsampler : public DownsamplerBase {
  //
public:
  Downsampler(class Environ *env,ULONG width,ULONG height);
  //
  virtual ~Downsampler(void);
  //
  // The actual downsampling process. To be implemented by the actual
  // classes inheriting from this. Coordinates are in the downsampled
  // block domain the block indices. Requires an output buffer that
  // will keep the downsampled data.
  virtual void DownsampleRegion(LONG bx,LONG by,LONG *buffer) const;
  //
};
///

///
#endif

