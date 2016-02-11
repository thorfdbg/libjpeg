/*
** This class provides an access path to the residual image in the
** form consistent to the block buffer / buffer control class, except
** that all the regular accesses go to the residual part. It does not
** manage buffers itself, but requires a block bitmap requester as
** base to forward the requests to.
**
** $Id: residualbuffer.cpp,v 1.4 2014/09/30 08:33:16 thor Exp $
**
*/
