/*************************************************************************

    This project implements a complete(!) JPEG (Recommendation ITU-T
    T.81 | ISO/IEC 10918-1) codec, plus a library that can be used to
    encode and decode JPEG streams. 
    It also implements ISO/IEC 18477 aka JPEG XT which is an extension
    towards intermediate, high-dynamic-range lossy and lossless coding
    of JPEG. In specific, it supports ISO/IEC 18477-3/-6/-7/-8 encoding.

    Note that only Profiles C and D of ISO/IEC 18477-7 are supported
    here. Check the JPEG XT reference software for a full implementation
    of ISO/IEC 18477-7.

    Copyright (C) 2012-2018 Thomas Richter, University of Stuttgart and
    Accusoft. (C) 2019-2020 Thomas Richter, Fraunhofer IIS.

    This program is available under two licenses, GPLv3 and the ITU
    Software licence Annex A Option 2, RAND conditions.

    For the full text of the GPU license option, see README.license.gpl.
    For the full text of the ITU license option, see README.license.itu.
    
    You may freely select between these two options.

    For the GPL option, please note the following:

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/

/*
** Definition of the Environment.
** 
** $Id: environment.hpp,v 1.12 2021/07/22 13:18:36 thor Exp $
**
** The environment holds structures for exception management without
** exceptions, and for memory management without a global new.
**
** (Almost) all objects should include this here.
**
*/

#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

/// Includes
#include "config.h"
#include "interface/types.hpp"
#include "interface/tagitem.hpp"
#include "interface/hooks.hpp"
#include "interface/parameters.hpp"
#include "std/stdlib.hpp"
#include "std/setjmp.hpp"
#include "debug.hpp"
#define NOREF(x) do {const void *y = &x;y=y;} while(0)
///

/// Forward declarations
class Environ;
class ExceptionRoot;
///

/// Exception
// This class keeps the data for one exception as it may propage upwards
// the stack.
class Exception {
  LONG                     m_lError;
  const char              *m_pWhat;
  LONG                     m_lLineNo;
  const char              *m_pSource;
  const char              *m_pDescription;
  //
public:
  Exception(void)
    : m_lError(0), m_pWhat(NULL), m_lLineNo(0), m_pSource(NULL), m_pDescription(NULL)
  { 
  }
  //
  Exception(LONG error,const char *what,LONG line,const char *src,const char *why)
    : m_lError(error), m_pWhat(what), m_lLineNo(line), m_pSource(src), m_pDescription(why)
  {
  }
  // 
  // Need to generate a copy constructor since the compiler will otherwise create
  // a call to memcpy which we cannot tolerate for Picclib.
  Exception(const Exception &org)
    : m_lError(org.m_lError), m_pWhat(org.m_pWhat), m_lLineNo(org.m_lLineNo),
      m_pSource(org.m_pSource), m_pDescription(org.m_pDescription)
  {
  }
  //
  // Need to generate an assignment operator since the compiler will otherwise create
  // a call to memcpy which we cannot tolerate for Picclib.
  class Exception &operator=(const Exception &org)
  {
    m_lError       = org.m_lError;
    m_pWhat        = org.m_pWhat;
    m_lLineNo      = org.m_lLineNo;
    m_pSource      = org.m_pSource;
    m_pDescription = org.m_pDescription;
    
    return *this;
  }
  //
  // Reset the exception, i.e. set to no error.
  void Reset(void)
  {
    m_lError = 0;
  }
  //
  // Return the error code of the exception.
  LONG ErrorOf(void) const
  {
    return m_lError;
  }
  //
  // Deliver the name of the object that caused the fault.
  const char *ObjectOf(void) const
  {
    return m_pWhat;
  }
  //
  // Return the line number within the file where the exception was caused.
  LONG LineOf(void) const
  {
    return m_lLineNo;
  }
  //
  // Return the name of the source file where the exception was caused.
  const char *SourceOf(void) const
  {
    return m_pSource;
  }
  //
  // Return a short description of the error or NULL for internal errors.
  const char *ReasonOf(void) const
  {
    return m_pDescription;
  }
  //
  // Check whether this exception is a no-op.
  bool isEmpty(void) const
  {
    return m_lError == 0;
  }
  //
  // Compare with another exception for equality.
  bool operator==(const class Exception &o) const
  {
    // The source code string is here used as a key. It is sufficient if
    // the same source file and the same line use the same static string,
    // which is surely correct.
    return (m_lLineNo == o.m_lLineNo) && (m_pSource == o.m_pSource);
  }
  //
  // Print out the exception in case we are debugging.
#if CHECK_LEVEL > 2
  void PrintException(const char *hdr) const
  {
    fprintf(stderr,
            "*** %s %ld in %s, line %ld, file %s\n"
            "*** Reason is: %s\n\n",
            hdr, long(m_lError),  m_pWhat,
            long(m_lLineNo), m_pSource,
            (m_pDescription)?(m_pDescription):("Internal error"));
  }
#else
  void PrintException(const char *) const
  {
  }
#endif
};
///

/// FATAL
// Forward a serious error by means of a segfault.
// FATAL is your choice to go if you do not have an environment and
// need to forward a *serious* error. Needless to say, this should
// **ONLY** happen for a non-zero check level. Never crash the end user!
//
#if CHECK_LEVEL > 0
extern void Fatal(const char *msg,const char *file,int line);
#endif
///

/// Defines
#ifdef HAVE_BUILTIN_EXPECT
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   x
#define unlikely(x) x
#endif
///

/// Design
/** Design
******************************************************************
** class ExceptionRoot                                          **
** Super Class: none                                            **
** Sub Classes: none                                            **
** Friends:     ExceptionStack, Environ                         **
******************************************************************

Purposes:       

The ExceptionRoot is the root node of the stack of exception
stack frames. Hence, each time you run a "try" a new 
ExceptionStack class is pushed onto the exception stack frames
whose base is kept by this class. Details about this can be
found in the ExceptionStack class design.

The ExceptionRoot expects for construction a first "dummy"
exception stack whose only purpose is to have a base to "link to"
and to avoid special cases in the stack management.

The ExceptionRoot also keeps the data of the last exception 
that was thrown, namely the error code, the source file location,
a description string and the name of the method that threw.

This data is needed for the client exception hook that wants
this information, and it is also needed for re-throwing an
exception, i.e. forwarding it to the next active exception.

The environment - see below - keeps two exception stacks, one
for warnings (that need not to be stacked anyhow, but so what)
and one for exceptions.

* */
///

/// ExceptionRoot
// This class defines the root of the exception stack, and
// keeps the data concerning this stack.
class ExceptionRoot {
  friend class ExceptionStack;
  friend class Environ;
  class ExceptionStack    *m_pActive;
  class Exception          m_Exception;
  //
  // Only the environment should create this.
  ExceptionRoot(class ExceptionStack *first)
    : m_pActive(first)
  { }
  //
  //
  // Return the last exception here.
  LONG LastException(const char *&description) const
  {
    description = m_Exception.ReasonOf();
    return m_Exception.ErrorOf();
  }
  //
  const class Exception &LastException(void) const
  {
    return m_Exception;
  }
  //
  // Print out the exception in case we are debugging.
  void PrintException(const char *hdr) const
  {
    m_Exception.PrintException(hdr);
  }
};
///

/// Design
/** Design
******************************************************************
** class ExceptionStack                                         **
** Super Class: none                                            **
** Sub Classes: none                                            **
** Friends:     Environ                                         **
******************************************************************

Purposes:       

The ExceptionStack keeps jump-back information for the exception-
less try-catch emulation within the libjpeg. Hence, each time
you "TRY", this object gets constructed, linked onto the list
of the exception stacks who are kept by the ExceptionRoot class,
and its jump buffer gets then constructed. Once you delete the
object, it unlinks itself from the ExceptionRoot. On a "THROW",
the topmost exception stack from the exception root is fetched,
unlinked from the exception stack and jumped to. 

Within the "catch" part, the exception stack constructed within
the current method must be unlinked as well since the "rethrow"
needs the exception stack of the previously active exepction stack,
i.e. the jump position of the upper "TRY" block, and not the jump-
block of the currently active "TRY"-block.

For "jumping", we use the C setjmp/longjmp function calls. Note,
however, that calling "SetJump" will not call the destructors of
the auto objects currently on the stack such that we *must not*
keep such objects. This is a *very important* design change from
previous releases of the library.

A "TRY" calls "setjmp", and checks the result code. If 0, the
buffer just got initialized and we continue execution with
the following instructions. A "CATCH" is simply the "else"
part of the "if" emulating the "TRY" - this is because setjmp
returns with non-zero if jumped at.

We furthermore need and "ENDTRY" that will dispose the active
exception stack frame. This is required in case we want to
use TRY/CATCH more than once within one scope, as otherwise names
would be clobbered - to be precise the name of the "invisible"
exception stack object. This is about the only visible change -
except capital letters - from the C++ syntax enforced by these
exception-less exceptions.

* */
///

/// ExceptionStack
// This class forms the entries in the exception stack
class ExceptionStack {
  friend class Environ;   // The environment must be able to access this
  //
  // Pointer to the previously active exception stack
  class ExceptionStack     *m_pPrevious;
  //
  // Pointer to the root of the exception stack
  class ExceptionRoot      *m_pRoot;  
  //  
  // The following assignment operator is really only
  // for the environment. It is only required to copy a new
  // exception stack over and nothing else. It should not
  // be called otherwise and is only present here to
  // shut up the compiler from generating a memcpy call.
  // BEWARE! This is not a full assignment operator
  class ExceptionStack &operator=(const class ExceptionStack &)
  {
    m_pPrevious = NULL;
    m_pRoot     = NULL;
#if CHECK_LEVEL > 0
    m_pFile     = NULL;
    m_iLine     = 0;
#endif
    return *this;
  }
  //
  // The following copy constructor is really only
  // for the environment. It is only required to copy a new
  // exception stack over and nothing else. It should not
  // be called otherwise and is only present here to
  // shut up the compiler from generating a memcpy call.
  // BEWARE! This is not a full assignment operator
  ExceptionStack(const class ExceptionStack &)
    : m_pPrevious(NULL), m_pRoot(NULL)
  {  
#if CHECK_LEVEL > 0
    m_pFile     = NULL;
    m_iLine     = 0;
#endif
  }
  //
public:
  // Create a currently unactive exception stack.
  ExceptionStack(void)
    : m_pPrevious(NULL), m_pRoot(NULL)
  { 
#if CHECK_LEVEL > 0
    m_pFile = NULL;
    m_iLine = 0;
#endif
  }
  //
  // The following must be public, unfortunately, or we couldn't
  // setjump here. Urghl.
  jmp_buf                  m_JumpDestination;
  //
#if CHECK_LEVEL > 0
  // The following two are for debugging purposes only
  const char              *m_pFile;
  int                      m_iLine;
#endif
  //
  // Constructor for all other objects
  inline ExceptionStack(class Environ *env);
  //
  // This class does not have a destructor that does an unlink.
  // Even though this looks unsafe not to do that here, it isn't.
  // The problem is that the exception stack is part of the lockable
  // object, and thus this destructor would be called as part of
  // the lockable destructor which would then potentially unlink
  // us from an already dead exception stack.
  //
  // Link the exception stack frame into the list of active stack frames
  // a detected exception will return to.
  inline void Link(class Environ *env);
  //
  // Unlink the exception from the stack (if not done so already)
  void Unlink(void)
  {
    assert(m_pRoot);
    m_pRoot->m_pActive = m_pPrevious;
  }
  //
  //
};
///

/// Design
/** Design
******************************************************************
** class Environ                                                **
** Super Class: none                                            **
** Sub Classes: none                                            **
** Friends:     none                                            **
******************************************************************

Purposes:       

The Environ class (short for "Environment" because I'm a lazy
guy) holds the system togehter. Its purpose is to provide very
basic system interface functions, memory allocation and memory
release functions, and it also keeps two "ExceptionRoots" for
the libjpeg "exceptions without exceptions(tm)" tricks. One of
the roots is the "Warning-Root" that collects warnings, the
other is the "Exception-Root" that keeps the stack of jump-
back (i.e. "TRY") locations. Warn() doesn't need such a stack
since Warn returns to the caller - unlike Throw() - but we do it
like this for consistency.

Obviously, the environment keeps also the exception hooks to be
called on exceptions resp. warnings, and provides the "Warn" and
"Throw" methods that are called for our exception mechanism.
These methods make use of the ExceptionRoot to run the exception
emulation.

Two pairs of memory allocation functions are provided: 
AllocMem/FreeMem and AllocVec/FreeVec. The difference between the
two is that AllocVec() keeps the size of the allocated memory
block internally, whereas AllocMem() doesn't. This means that a
FreeMem() has to know the size of the released memory block, which
avoids unnecessary overhead especially for small memory blocks,
typically matrices of a primitive data type. Obviously, AllocVec
and FreeVec() are build on top of AllocMem and FreeMem().

In case you wonder about these names - well, these are the same
functions the Amiga Os provides. There's no place like home. (-:

The environment must be build at the very beginning of the library
entry point, as it is required for everything else. Therefore, we
have a chicken-or-egg problem here: The environment itself must
be available to allocate an Environ class for the jpeg root class.
To resolve this problem, we first build a temporary environment as
auto-object on the stack, which gets the user-provided tag-lists
passed in. These tag lists initialize the hooks for memory
allocation and more. Then we allocate a new environment from this
temporary object and copy the object over. We may then dispose
the auto object as soon as we leave the "construct" method of the
root class. For this nasty trick, an overloaded assignment operator
and a copy-constructor are provided. Unfortunately, exception
stacks must be carried over such that we cannot keep the source
of the construction intact. Hence, copying an environment has to
touch the source. Yuck!

* */
///

/// The Environment: class Environ
// This class keeps the connection to the outher world. It knows the
// exception base, and the connection to the memory allocation and
// release functions.
class Environ {  
  friend ExceptionStack::ExceptionStack(class Environ *env);
  friend void ExceptionStack::Link(class Environ *env);
  //
  //
public:
  // Maximum alignment restriction: Defines how aligned a type
  // can get at most. This guy here is never allocated, but its
  // size defines the maximum alignment the types must have.
  union Align {
    UBYTE   a_byte;
    UWORD   a_word;
    ULONG   a_int;
    FLOAT   a_float;
    DOUBLE  a_double;
    UQUAD   a_quad;
    APTR    a_ptr;
  };
  //
private:
  // 
  // The first active exception (top exception stack entry)
  class ExceptionStack   m_First;
  // and the root of the exception tree
  class ExceptionRoot    m_Root;
  //
  // The following class is here to collect warnings rather than
  // exceptions.
  class ExceptionRoot    m_WarnRoot;
  //
  // The memory pool, manages small memory allocations.
  class MemoryPool      *m_pPool;
  //
  // In case this environment is a thread-local environment,
  // here's the root.
  class Environ         *m_pParent;
  //
  // we furthermore need hooks for the memory allocation. They follow
  // here:
  //
  // Allocate memory:
  struct JPG_Hook       *m_pAllocationHook;
  // Release memory:
  struct JPG_Hook       *m_pReleaseHook;
  //
  // Pass exception:
  struct JPG_Hook       *m_pExceptionHook;
  // Pass warning:
  struct JPG_Hook       *m_pWarningHook;
  //
  // Hooks for multithreading related activites:
  // Thread creation, mutex and semaphores.
  struct JPG_Hook       *m_pThreadHook;
  struct JPG_Hook       *m_pMutexHook;
  //
  // For optimal performance, we pre-build the tag lists for
  // the allocation and release hooks:
  //
  // For allocation:
  struct JPG_TagItem     m_AllocationTags[4];
  // for release:
  struct JPG_TagItem     m_ReleaseTags[4];
  //
  // Pre-allocated tags for the exceptions:
  struct JPG_TagItem     m_ExceptionTags[7];
  // and for the warnings
  struct JPG_TagItem     m_WarningTags[7];
  //
  // If the following bool is set, then the library tries
  // to suppress multiple identical warnings.
  bool                   m_bSuppressMultiple;
  //
  // The number of warnings we keep at most. This should be sufficient
  // for most runs.
  enum {
    WarnQueueSize = 16
  };
  //
  // The following is a data base of warnings. It is a simple 
  // move-to-front list of the recently found warnings, nothing fancy.
  class Exception        m_WarnQueue[WarnQueueSize];
  //
  // Forward a warning or an exception to the supplied hook
  void ForwardMessage(struct JPG_Hook *hook,struct JPG_TagItem *tags,
                      const class Exception &exc);
  //
  // Internal memory allocation functions, not for public use.
  inline void *CoreAllocMem(ULONG bytesize,ULONG reqments);
  inline void CoreFreeMem(void *mem,ULONG bytesize);
  //
  // Check whether the given warning (at the line and source file) is already
  // in the warning database. In case it is, return false. Otherwise, enter
  // it to the data base and return true.
  bool isWarned(const class Exception &e);
  //
  // Now to the public parts: Constructors, destructors.
public:
  //
  // Construct an environment from a tag list. This must be called
  // once for the JPG-Class.
  Environ(struct JPG_TagItem *tags = NULL);
  //
  // An assignment operator
  class Environ& operator=(class Environ &env);
  //
  // A copy-constructor: Beware, this makes the copied object unusable!
  Environ(class Environ &env)  
    : m_First(), m_Root(&m_First), m_WarnRoot(&m_First), 
      m_pParent(NULL), m_bSuppressMultiple(true)
  {
    *this = env;
  }
  //
  // Clone the exception from another exception to create an identically working
  // copy for a side-thread, but with an empty exception stack.
  Environ(class Environ *env);
  //
  // This is part of the delayed construction: Provide the memory pool. Should
  // be called immediately after bootstrapping the environment. May throw.
  void BuildPool(void);
  //
  // Destructor
  ~Environ(void);
  //
  // Return the active exception stack for throw-ing.
  class ExceptionStack *ActiveExcOf(void) const 
  {
    return m_Root.m_pActive;
  }
  //
  // Clean the warn queue/history, thus report all warnings from this
  // point on again.
  void CleanWarnQueue(void);
  //
  // Memory allocation and deallocation functions:
  //
  // Takes a byte size and a requirement factor. The latter is not
  // yet in use. Note that it is slightly more effective not to
  // use a default argument here.
  void *AllocMem(size_t bytesize,ULONG requirements);
  void *AllocMem(size_t bytesize);
  //
  // The same again, but this call remembers the size
  void *AllocVec(size_t bytesize,ULONG requirements);
  void *AllocVec(size_t bytesize); 
  //
  // Free memory: Takes the memory pointer and the size of memory
  // to be deallocated.
  void FreeMem(void *mem,size_t bytesize);
  // Free a thing allocated by AllocVec, no bytesize required here.
  void FreeVec(void *mem);
  //
  // For the following two, Arguments are as this:
  // error code from exception.hpp,
  // name of the method that throws
  // line number that throws
  // name of the source file that throws
  // description of the failure condition
  //
  // Forward a warning
  void Warn(const LONG error, const char *what,const LONG line, 
            const char *where, const char *description);
  //
  void Warn(const class Exception &ex);
  //
  // Deliver the last caugt exception as warning only.
  void LowerToWarning(void);
  //
  // Throw an exception
  void Throw(const LONG error, const char *what, const LONG line,
             const char *where, const char *description) NORETURN;
  //
  // Throw an exception class directly.
  void Throw(const class Exception &ex) NORETURN;
  //
  // Rethrow the last exception to the next higher hierarchy
  void ReThrow(void) NORETURN;
  // 
  // Get information about the environment.
  void GetInformation(struct JPG_TagItem *tags) const;
  //
  // Deliver the last error again over the exception hook
  void PostLastError(void);
  //
  // Deliver the last warning again over the warning hook
  void PostLastWarning(void);
  //
  // The following is for debugging purposes only:
  // Print out the last exception, resp. the last warning
  void PrintException(void) const
  {
    m_Root.PrintException("Error");
  }
  // Print out the last warning - this is also for debugging
  // purposes only.
  void PrintWarning(void) const
  {
    m_WarnRoot.PrintException("Warning");
  }
  //
  // Return the last (pending) exception
  LONG LastException(const char *&description) const
  {
    return m_Root.LastException(description);
  }
  //
  // Return the last (pending) warning
  LONG LastWarning(const char *&description) const
  {
    return m_WarnRoot.LastException(description);
  }
  //
  // Return the last exception class.
  const class Exception &LastException(void) const
  {
    return m_Root.LastException();
  }
  //
  // Return the last warning.
  const class Exception &LastWarning(void) const
  {
    return m_WarnRoot.LastException();
  }
  //
  // In case several warnings are buffered, advance to the next buffered
  // warning in this environment. Requires JPGTAG_EXC_SUPPRESS_IDENTICAL
  // to be set.
  void NextWarning(void);
  //
  // Merge the contents of our warning queue from the
  // warning queue of the given environment.
  void MergeWarningQueueFrom(class Environ *p);
  //
  // Test whether the exception stack is empty.
#if CHECK_LEVEL > 0
  void TestExceptionStack(void)
  {
    assert(m_Root.m_pActive);
    const class ExceptionStack *exc = m_Root.m_pActive;
    assert(exc->m_pPrevious == NULL);
  }
#endif
  //
};
///

/// ExceptionStack::ExceptionStack
ExceptionStack::ExceptionStack(class Environ *env)
  : m_pPrevious(env->m_Root.m_pActive), m_pRoot(&(env->m_Root))
{
  // Make our exception the topmost active one.
  env->m_Root.m_pActive = this;
}
///

/// ExceptionStack::Link
void ExceptionStack::Link(class Environ *env)
{
  m_pPrevious           = env->m_Root.m_pActive;
  m_pRoot               = &(env->m_Root);
  env->m_Root.m_pActive = this;
}
///

/// Exception macros
// Nasty macros for exception handling emulation
// A try macro of the try-catch construction. This guy assumes
// that the class has a member m_pEnviron pointing to the above
// environment vector.
//
// See the design of the ExceptionStack how these macros work.
//
#if CHECK_LEVEL > 0 && defined(PTHREAD_DEBUGGING)
#define JPG_TRY                                \
{ class ExceptionStack __exc__(m_pEnviron);    \
  __exc__.m_pFile = __FILE__;                  \
  __exc__.m_iLine = __LINE__;                  \
  m_pEnviron->TestCaller();                    \
  if (likely(setjmp(__exc__.m_JumpDestination) == 0))
#else
#define JPG_TRY                                \
{ class ExceptionStack __exc__(m_pEnviron);    \
  if (likely(setjmp(__exc__.m_JumpDestination) == 0))
#endif

// A catch macro: This is simply the else-part of the setjmp above.
#define JPG_CATCH else {
// Throw an error: Requires an error code, the name of the object
// we throw within and a failure description. Requires the environment.
#define JPG_THROW(err,obj,des) m_pEnviron->Throw(JPGERR_ ## err,obj,__LINE__,__FILE__,des)
// Throw an error: Required an error code as integer, the name of
// the object we throw within and a failure description. Requires
// the environment.
#define JPG_THROW_INT(err,obj,des) m_pEnviron->Throw(err, obj, __LINE__, __FILE__, des)

// Re-throw the active exception again to forward the exception 
#define JPG_RETHROW m_pEnviron->ReThrow()

// End of a try-catch block
#define JPG_ENDTRY } __exc__.Unlink();}

// Unlink and return from a TRY.
#define JPG_RETURN __exc__.Unlink();return;

// Pass a warning
#define JPG_WARN(err,obj,des) m_pEnviron->Warn(JPGERR_ ## err,obj,__LINE__,__FILE__,des)
///

/// Memory allocation macros
// Allocate generic memory, remember its size
#define JPG_MALLOC(x)  m_pEnviron->AllocVec(x)
//
// Release the memory allocated by MALLOC
#define JPG_FREE(x)    m_pEnviron->FreeVec(x)
///

/// Design
/** Design
******************************************************************
** class JObject                                                **
** Super Class: none                                            **
** Sub Classes: JKeeper, and lots and lots others               **
** Friends:     none                                            **
******************************************************************

Purposes:       

If an object is created by means of "new", a global new operator
is called. Unfortunately, when overloading this operator to the
AllocMem of the environment described above, we would also over-
load the new operator of programs that are linked against this
library, which is clearly a no-no.

The purpose of the JObject is to provide a new operator local
to the class that wants to be new'd that forwards the request
to the environment. Unfortunately, we do not want global 
variables, which means that new() needs to pass-in an argument.
C++ allows such constructs, known as the "arena new" with the
"arena" here called the "environment". The JObject therefore
implements an "arena new" with environment argument that
forwards the allocation to the memory allocation functions of
the Environ class.

Unfortunately, delete() never gets any "arena" argument, so we 
need to keep the environment for the delete() somewhere on the 
allocated memory and need to extract it from there.

Similar functions are provided for allocating and releasing
arrays.

Hence, *every single class* within the JPG tree that wants to
be "new'd" must be inherited directly or indirectly from
the JObject as every class *requires* a private new; global 
operator new is out, remember! Therefore, we do not document
explicit or implicit dependency from JObject in all other
objects.

This also means that every class that wants to allocate(!)
another class requires an environment pointer. This issue is
resolved by the next class. The idea is to keep memory 
requirements for "leaf classes" as low as possible, hence avoid
the overhead of keeping another environment pointer within
them.

* */
///

/// JObject
// The JObject is the base object for all objects within the
// JPG-library that want to throw or to allocate memory.
// All objects must be derived from the JObject
class JObject {
private:
  // Make Operator New without arguments private here.
  // They must not be used.
  void *operator new[](size_t)
  {
    assert(false);
    return (void *)1; // GNU doesn't accept NULL here. This should throw as well.
  }
  void *operator new(size_t)
  {
    assert(false);
    return (void *)1;
  }
  //
  // Private structure for effective arena-new management:
  //
  // Due to bugs in both the GCC and the VS, we must keep the
  // size in the memory holder as well. The size parameter of
  // the delete operator is just broken (a little broken on GCC,
  // a lot broken on VS).
  // This is still slightly more effective than using AllocVec
  // here.
  // *BIG SIGH*
  union MemoryHolder {
    // This union here enforces alignment
    // of the data.
    struct {
      // back-pointer to the environment
      class Environ     *mh_pEnviron;
      size_t             mh_ulSize;
    }                    mh;
    // The following are here strictly for alignment
    // reasons.
    union Environ::Align mh_Align;
  };
  //
public:
  // An overloaded "new" operator to allocate memory from the
  // environment.
  static void *operator new[](size_t size,class Environ *env)    
  {
    union MemoryHolder *mem;
    // Keep the memory holder here as well
    size += sizeof(union MemoryHolder);
    // Now allocate the memory from the environment.
    mem = (union MemoryHolder *)env->AllocMem(size);
    // Install the environment pointer and (possibly) the size
    mem->mh.mh_pEnviron = env;
    mem->mh.mh_ulSize   = size;
    return (void *)(mem + 1);
  }
  //
  static void *operator new(size_t size,class Environ *env)
  {
    union MemoryHolder *mem;
    // Keep the memory holder here as well
    size += sizeof(union MemoryHolder);
    // Now allocate the memory from the environment.
    mem = (union MemoryHolder *)env->AllocMem(size);
    // Install the environment pointer and (possibly) the size
    mem->mh.mh_pEnviron = env;
    mem->mh.mh_ulSize   = size;
    return (void *)(mem + 1);
  }
  //
  //
  // An overloaded "delete" operator to remove memory from the
  // environment.
  static void operator delete[](void *obj)
  {
    if (obj) {
      union MemoryHolder *mem = ((union MemoryHolder *)(obj)) - 1;
      mem->mh.mh_pEnviron->FreeMem(mem,mem->mh.mh_ulSize);
    }
  }
  //
  //
  static void operator delete(void *obj)
  {
    if (obj) {
      union MemoryHolder *mem = ((union MemoryHolder *)(obj)) - 1;
      mem->mh.mh_pEnviron->FreeMem(mem,mem->mh.mh_ulSize);
    }
  }
};
///

/// Explicit
// The following base class should be derived from
// to make copy-construction and assignment impossible.
// This is the default recommended method for JPG objects
class Explicit {
  //
  // Hide copy constructor and assignment operator
  Explicit(const class Explicit &)
  { }
  class Explicit &operator=(const class Explicit &)
  {
    return *this;
  }
  //
public:
  // But make default-constructable.
  Explicit(void)
  { }
};
///

/// Ambigious new workaround makro
// The following macro can be used to make an operator
// new unambigious in case of a broken compiler. (VS 6, gcc 2.95.xx)
#ifdef AMBIGIOUS_NEW_BUG
#define BORROW_NEW(base) \
public:  \
void *operator new(size_t size,class Environ *env) { return base::operator new(size,env); } \
void operator delete(void *obj) { base::operator delete(obj); } \
private:
#else
#define BORROW_NEW(base)
#endif
///

/// Design
/** Design
******************************************************************
** class JKeeper                                                **
** Super Class: JObject                                         **
** Sub Classes: lots and lots                                   **
** Friends:     none                                            **
******************************************************************

Purposes:

This is a direct descendent of the JObject class and therefore
has the ability to be "allocated". However, it also provides its
children the ablility to "allocate". Simple JObjects do not keep
an Environ pointer and hence lack the argument passed to the
"arena new". Hence, the one and only member of a JObject is a
pointer to the environment which must be carried along for memory
allocation, and which must be passed over to the constructors
of all children - if the childs want to be able to "allocate".

Hence, a "JKeeper" is an extended version of "JObject" that
also allows "allocation", not just "to be allocated". Major 
classes within the J2k class tree must be descendents of this
class. Almost everything must depend on this, and we should
not document dependence explicitly.

Besides that, the exception management functions also require
an Environ to reach the exception root for the exception stack;
hence, you may only "Throw" within a "JKeeper derived" object,
or if you get hold of an environment in another way.

* */
///

/// JExtender
// This object extends a class by an environment pointer.
// This is actually a compiler workaround for g++-2.95
// that otherwise won't accept a doubly imported operator new
// from the JObject and the JKeeper
class JExtender {
protected:
  // Pointer to the environment: This is the only reason why this
  // object should be required.
  class Environ *m_pEnviron;
  // Note the precise writing: This is required by the exception
  // management stuff.
  //
  // Constructors: also protected because this object is not to be
  // constructed without a class that is derived from it.
  JExtender(class Environ *env) 
    : m_pEnviron(env)
  { }
  //
public:
  // Return the environment of this object; this could be useful
  // in case we need this pointer by one of the children for
  // construction.
  class Environ *EnvironOf(void) const 
  {
    return m_pEnviron;
  }
};
///

/// JKeeper
// Similar to a JObject, except that it also keeps a pointer to the
// environment. This is for easy "throw and the like".
class JKeeper : public JObject, public JExtender {
protected:
  //
  // Constructors: also protected because this object is not to be
  // constructed without a class that is derived from it.
  JKeeper(class Environ *env)
    : JExtender(env)
  { }
  //
};
///

/// global new operator is now obsolete, at least for the check level
#if CHECK_LEVEL > 0
void *operator new(size_t);
void *operator new[](size_t);
#endif
///

///
#endif
