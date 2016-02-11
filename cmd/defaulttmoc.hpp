/*
** This file provides a simple TMO that provides fine quality and natural
** look in most cases. It is mostly a global Reinhard operator:
** Erik Reinhard and Kate Devlin. Dynamic Range Reduction Inspired by
** Photoreceptor Physiology.  IEEE Transactions on Visualization and
** Computer Graphics (2004).
**
** This is the default TMO for profile C.
**
** $Id: defaulttmoc.hpp,v 1.2 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CMD_DEFAULTTMOC_HPP
#define CMD_DEFAULTTMOC_HPP

/// Includes
#include "interface/types.hpp"
#include "std/stdio.hpp"
///

/// Prototypes
extern void BuildToneMapping_C(FILE *in,int w,int h,int depth,int count,UWORD tonemapping[65536],
			       bool flt,bool bigendian,bool xyz,int hiddenbits);
///

///
#endif
