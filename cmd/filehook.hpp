/*
** This module implements the IO-Hook function that reads and writes
** the encoded data.
**
** $Id: filehook.hpp,v 1.2 2014/09/30 08:33:15 thor Exp $
**
*/

#ifndef CMD_FILEHOOK_HPP
#define CMD_FILEHOOK_HPP

/// Includes
#include "interface/types.hpp"
///

/// Forwards
struct JPG_Hook;
struct JPG_TagItem;
///

/// Prototypes
extern JPG_LONG FileHook(struct JPG_Hook *hook, struct JPG_TagItem *tags);
///

///
#endif
