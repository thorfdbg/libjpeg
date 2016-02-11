/*
 * Type definition: Some system independent type definitions
 * (thor's pecularities)
 * $Id: types.cpp,v 1.7 2014/09/30 08:33:17 thor Exp $
 *
 * The following header defines basic types to be used in the J2K interface
 * routines. Especially, this file must be adapted if your compiler has
 * different ideas what a "unsigned long" is as we *MUST* fix the width
 * of elementary data types. Especially, do not use types not defined here for
 * interface glue routines.
 *
 * This is the "internal" header file defining internal types, importing the
 * types from the external "jpgtypes" header.
 */

