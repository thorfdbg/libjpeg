// There needs to be SOME input file for any project.  Could be empty.

// But I also wish to incur DLL exporting on the main interface classes.

// It should THEORETICALLY be enough to do this:

#include "interface/jpeg.hpp"
#include "interface/hooks.hpp"
#include "interface/tagitem.hpp"

// Together with the global define J2K_EXPORT=__declspecl(dllexport) (see
// Project > Settings > C/C++ > Preprocessor > Preprocessor definitions:),
// that should cause the proper classes to be exported.  At least according
// to the MS Visual Studio's builtin documentation.  This scheme replaces
// .def files, which don't work well with C++ (name decoration issue).

// In practice, that doesn't work because the compiler turns a class EXPORT
// into an EXPORT for each member.  But it doesn't emit any code
// for classes/members that are simply declared.  It only emits code if a
// DEFINITION is seen.  And if there isn't any code, the linker can't see
// the export tag.  

// Actually, that whole problem is quite understandable, and the setup is
// pretty sane.  But it leaves us with the problem of how to tell the linker
// to export these classes.

// I haven't found ANY good workaround.  Candidates are things like:
//		- class tempXCC : public /* virtual */ J2K {};
//	    - void  tempXFC (void) { J2K wc; }
// The second DOES at least export SOME members (namely constructors,
// destructors and class info).

// The following is the only thing I found that works.  Drawback is that now
// I HAVE to set all the Compiler options JUST like while compiling the static
// lib, and I won't get a warning if I don't (but the .dll won't work...).
// Real error prone.

#include "interface/jpeg.hpp"
