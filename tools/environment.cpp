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
** $Id: environment.cpp,v 1.11 2022/06/14 06:18:30 thor Exp $
**
** The environment holds structures for exception management without
** exceptions, and for memory management without a global new.
**
**
*/

/// Includes
#include "environment.hpp"
#include "interface/parameters.hpp"
#include "std/stdlib.hpp"
#include "std/stddef.hpp"
#include "std/string.hpp"
#include "tools/debug.hpp"
///

/// Defines
// Define this it get a histogram of the memory usage
//#define HIST 1
//
// Define whether we want to print allocations
//#define DEBUG_ALLOC
//
#if CHECK_LEVEL > 0 && !defined(USE_VALGRIND)
#define MUNGE_MEM 1
#endif
#if !defined(USE_VALGRIND)
#define USE_POOL 1
#endif
///

/// Externals
///

/// Memory counter
#if CHECK_LEVEL > 0
static ULONG totalmem    = 0;
static ULONG maxmem      = 0;
ULONG malloccount        = 0;
#endif

#ifdef HIST
#include "std/stdio.hpp"
#define SMALL_MEM_LIMIT 16384
static ULONG memhist[32];
static ULONG smallmem[SMALL_MEM_LIMIT];
static ULONG lifetime[SMALL_MEM_LIMIT];
static ULONG allocstamp[SMALL_MEM_LIMIT];
static ULONG lifetimer = 0;
#endif
///

/// BreakPoint
#if DEBUG_LEVEL > 0 || defined(TIME_ME)
#include "std/stdio.hpp"
void BreakPoint(void)
{

}
#endif
///

/// Memory histogram support
#ifdef HIST
static void RecordMemAlloc(size_t bytesize)
{ 
  unsigned long i = 0;
  
  lifetimer++;
  while(bytesize >= (1UL<<(i+1)))
    ++i;
#if HAVE_ATOMIC_ADDSUB
  __sync_add_and_fetch(memhist+i,1);
#else
  memhist[i]++;
#endif
  if (bytesize<SMALL_MEM_LIMIT) {
#if HAVE_ATOMIC_ADDSUB
    __sync_add_and_fetch(smallmem+bytesize,1);
#else
    smallmem[bytesize]++;
#endif
    allocstamp[bytesize] = lifetimer;
  }
}

static void RecordMemFree(size_t bytesize)
{
  lifetimer++;

  if (bytesize < SMALL_MEM_LIMIT) {
    ULONG t = lifetimer - lifetime[bytesize];
    
    if (allocstamp[bytesize] > 0 && (lifetime[bytesize] == 0 || t < lifetime[bytesize])) {
      lifetime[bytesize]   = t;
      allocstamp[bytesize] = 0;
    }
  }
}

static void PrintHist(void)
{
  unsigned long i;
  
  for(i=0;i<32;i++) {
    printf("Allocations for memory of chunk size between %10lu and %10lu: %lu\n",
           1UL<<i,1UL<<(i+1),(unsigned long)memhist[i]);
  }
  for(i=0;i<SMALL_MEM_LIMIT;i++) {
    if (smallmem[i])
      printf("Allocation of %3lu bytes are: %lu\n",i,(unsigned long)(smallmem[i]));
  }
  for(i=0;i<SMALL_MEM_LIMIT;i++) {
    if (lifetime[i])
      printf("Life time of %3lu bytes are: %lu\n",i,(unsigned long)(lifetime[i]));
  }
}
#endif
///

/// Environ::Environ
// Tag-List constructor of the environment
Environ::Environ(struct JPG_TagItem *tags)
  : m_First(), m_Root(&m_First), m_WarnRoot(&m_First), 
    m_pParent(NULL)
{
  // Now fill in the hooks from the supplied tag list
  if (tags) {
    m_pAllocationHook   = (struct JPG_Hook *)tags->GetTagPtr(JPGTAG_MIO_ALLOC_HOOK);
    m_pReleaseHook      = (struct JPG_Hook *)tags->GetTagPtr(JPGTAG_MIO_RELEASE_HOOK);
    //
    m_pExceptionHook    = (struct JPG_Hook *)tags->GetTagPtr(JPGTAG_EXC_EXCEPTION_HOOK);
    m_pWarningHook      = (struct JPG_Hook *)tags->GetTagPtr(JPGTAG_EXC_WARNING_HOOK); 
    m_bSuppressMultiple = tags->GetTagData(JPGTAG_EXC_SUPPRESS_IDENTICAL)?true:false;
    //
  } else {
    m_pAllocationHook   = NULL;
    m_pReleaseHook      = NULL;
    m_pExceptionHook    = NULL;
    m_pWarningHook      = NULL; 
    m_bSuppressMultiple = true;
  }
  //
  //
  // Now fill in the tags for the allocator
  m_AllocationTags[0].ti_Tag = JPGTAG_MIO_SIZE;
  m_AllocationTags[1].ti_Tag = JPGTAG_MIO_TYPE;
  m_AllocationTags[2].ti_Tag = JPGTAG_MIO_ALLOC_USERDATA;
  m_AllocationTags[2].ti_Data.ti_pPtr = tags?tags->GetTagPtr(JPGTAG_MIO_ALLOC_USERDATA):NULL;
  m_AllocationTags[3].ti_Tag = JPGTAG_TAG_DONE;
  //
  // now go for the tags for the memory release: Do not fill in the
  // size tag unless we really "keep" it.
  m_ReleaseTags[0].ti_Tag    = JPGTAG_MIO_SIZE;
  m_ReleaseTags[1].ti_Tag    = JPGTAG_MIO_MEMORY;
  m_ReleaseTags[2].ti_Tag    = JPGTAG_MIO_RELEASE_USERDATA;
  m_ReleaseTags[2].ti_Data.ti_pPtr = tags?tags->GetTagPtr(JPGTAG_MIO_RELEASE_USERDATA):NULL;
  m_ReleaseTags[3].ti_Tag    = JPGTAG_TAG_DONE;
  //
  // Fill in the exception tags
  m_ExceptionTags[0].ti_Tag  = JPGTAG_EXC_ERROR;
  m_ExceptionTags[1].ti_Tag  = JPGTAG_EXC_CLASS;
  m_ExceptionTags[2].ti_Tag  = JPGTAG_EXC_LINE;
  m_ExceptionTags[3].ti_Tag  = JPGTAG_EXC_SOURCE;
  m_ExceptionTags[4].ti_Tag  = JPGTAG_EXC_DESCRIPTION;
  m_ExceptionTags[5].ti_Tag  = JPGTAG_EXC_EXCEPTION_USERDATA;
  m_ExceptionTags[5].ti_Data.ti_pPtr = tags?tags->GetTagPtr(JPGTAG_EXC_EXCEPTION_USERDATA):NULL;
  m_ExceptionTags[6].ti_Tag  = JPGTAG_TAG_DONE;
  //
  // and finally for the warning hook
  m_WarningTags[0].ti_Tag    = JPGTAG_EXC_ERROR;
  m_WarningTags[1].ti_Tag    = JPGTAG_EXC_CLASS;
  m_WarningTags[2].ti_Tag    = JPGTAG_EXC_LINE;
  m_WarningTags[3].ti_Tag    = JPGTAG_EXC_SOURCE;
  m_WarningTags[4].ti_Tag    = JPGTAG_EXC_DESCRIPTION;
  m_WarningTags[5].ti_Tag    = JPGTAG_EXC_WARNING_USERDATA;
  m_WarningTags[5].ti_Data.ti_pPtr = tags?tags->GetTagPtr(JPGTAG_EXC_WARNING_USERDATA):NULL;
  m_WarningTags[6].ti_Tag    = JPGTAG_TAG_DONE;
  //
  //
  CleanWarnQueue();
}
///

/// Assignment operator for the environment
class Environ &Environ::operator=(class Environ &env)
{
  class ExceptionStack *es,*prev;
  //
  m_First        = ExceptionStack();
  m_Root         = ExceptionRoot(&m_First);
  m_WarnRoot     = ExceptionRoot(&m_First);
  //
  // Copy the parent node over.
  m_pParent      = env.m_pParent;
  //
  // Now carry the active exception stack frames over
  prev           = NULL;
  es             = env.m_Root.m_pActive;
  while(es->m_pPrevious) { 
    // repeat until we find the first dummy exception stack frame
    prev         = es;
    if (m_Root.m_pActive->m_pPrevious == NULL)
      m_Root.m_pActive = es;
    es->m_pRoot  = &(this->m_Root);
    es           = es->m_pPrevious;
  }
  // Now link the "prev" into our first if we have any
  if (prev) 
    prev->m_pPrevious        = &(m_First);
  //
  // Now fill in the hooks from the supplied tag list
  m_pAllocationHook          = env.m_pAllocationHook;
  m_pReleaseHook             = env.m_pReleaseHook;
  //
  m_pExceptionHook           = env.m_pExceptionHook;
  m_pWarningHook             = env.m_pWarningHook;
  //
  // Now fill in the tags for the allocator
  m_AllocationTags[0].ti_Tag = JPGTAG_MIO_SIZE;
  m_AllocationTags[1].ti_Tag = JPGTAG_MIO_TYPE;
  m_AllocationTags[2]        = env.m_AllocationTags[2];
  m_AllocationTags[3].ti_Tag = JPGTAG_TAG_DONE;
  //
  // now go for the tags for the memory release
  m_ReleaseTags[0].ti_Tag    = env.m_ReleaseTags[0].ti_Tag;
  m_ReleaseTags[1].ti_Tag    = JPGTAG_MIO_MEMORY;
  m_ReleaseTags[2].ti_Tag    = JPGTAG_MIO_RELEASE_USERDATA;
  m_ReleaseTags[2]           = env.m_ReleaseTags[2];
  m_ReleaseTags[3].ti_Tag    = JPGTAG_TAG_DONE;
  //
  // Fill in the exception tags
  m_ExceptionTags[0].ti_Tag  = JPGTAG_EXC_ERROR;
  m_ExceptionTags[1].ti_Tag  = JPGTAG_EXC_CLASS;
  m_ExceptionTags[2].ti_Tag  = JPGTAG_EXC_LINE;
  m_ExceptionTags[3].ti_Tag  = JPGTAG_EXC_SOURCE;
  m_ExceptionTags[4].ti_Tag  = JPGTAG_EXC_DESCRIPTION;
  m_ExceptionTags[5]         = env.m_ExceptionTags[5];
  m_ExceptionTags[6].ti_Tag  = JPGTAG_TAG_DONE;
  //
  // and finally for the warning hook
  m_WarningTags[0].ti_Tag    = JPGTAG_EXC_ERROR;
  m_WarningTags[1].ti_Tag    = JPGTAG_EXC_CLASS;
  m_WarningTags[2].ti_Tag    = JPGTAG_EXC_LINE;
  m_WarningTags[3].ti_Tag    = JPGTAG_EXC_SOURCE;
  m_WarningTags[4].ti_Tag    = JPGTAG_EXC_DESCRIPTION;
  m_WarningTags[5].ti_Tag    = JPGTAG_EXC_WARNING_USERDATA;
  m_WarningTags[5]           = env.m_WarningTags[5];
  m_WarningTags[6].ti_Tag    = JPGTAG_TAG_DONE;
  
  // Clean the exception root as indicator
  // whether we shall check the memory that remains allocated.
  env.m_Root.m_pActive       = NULL;
  //
  CleanWarnQueue();
  //
  return *this;
}
///

/// Environ::Environ
// Clone the exception from another exception to create an identically working
// copy for a side-thread, but with an empty exception stack.
Environ::Environ(class Environ *env)
  : m_First(), m_Root(&m_First), m_WarnRoot(&m_First), m_pParent(env)
{  
  //
  // Check whether we are creating environment trees, i.e. the
  // parent is not the root. This is not supported as it complicates
  // the warning/exception handling structure. Threads should only
  // be created by the root.
  assert(m_pParent);
  assert(m_pParent->m_pParent == NULL);
  //
  // Now fill in the hooks from the supplied tag list
  m_pAllocationHook          = env->m_pAllocationHook;
  m_pReleaseHook             = env->m_pReleaseHook;
  //
  m_pExceptionHook           = env->m_pExceptionHook;
  m_pWarningHook             = env->m_pWarningHook;
  //
  m_bSuppressMultiple        = env->m_bSuppressMultiple;
  //
  // Now fill in the tags for the allocator
  m_AllocationTags[0].ti_Tag = JPGTAG_MIO_SIZE;
  m_AllocationTags[1].ti_Tag = JPGTAG_MIO_TYPE;
  m_AllocationTags[2]        = env->m_AllocationTags[2];
  m_AllocationTags[3].ti_Tag = JPGTAG_TAG_DONE;
  //
  // now go for the tags for the memory release
  m_ReleaseTags[0].ti_Tag    = env->m_ReleaseTags[0].ti_Tag;
  m_ReleaseTags[1].ti_Tag    = JPGTAG_MIO_MEMORY;
  m_ReleaseTags[2].ti_Tag    = JPGTAG_MIO_RELEASE_USERDATA;
  m_ReleaseTags[2]           = env->m_ReleaseTags[2];
  m_ReleaseTags[3].ti_Tag    = JPGTAG_TAG_DONE;
  //
  // Fill in the exception tags
  m_ExceptionTags[0].ti_Tag  = JPGTAG_EXC_ERROR;
  m_ExceptionTags[1].ti_Tag  = JPGTAG_EXC_CLASS;
  m_ExceptionTags[2].ti_Tag  = JPGTAG_EXC_LINE;
  m_ExceptionTags[3].ti_Tag  = JPGTAG_EXC_SOURCE;
  m_ExceptionTags[4].ti_Tag  = JPGTAG_EXC_DESCRIPTION;
  m_ExceptionTags[5]         = env->m_ExceptionTags[5];
  m_ExceptionTags[6].ti_Tag  = JPGTAG_TAG_DONE;
  //
  // and finally for the warning hook
  m_WarningTags[0].ti_Tag    = JPGTAG_EXC_ERROR;
  m_WarningTags[1].ti_Tag    = JPGTAG_EXC_CLASS;
  m_WarningTags[2].ti_Tag    = JPGTAG_EXC_LINE;
  m_WarningTags[3].ti_Tag    = JPGTAG_EXC_SOURCE;
  m_WarningTags[4].ti_Tag    = JPGTAG_EXC_DESCRIPTION;
  m_WarningTags[5].ti_Tag    = JPGTAG_EXC_WARNING_USERDATA;
  m_WarningTags[5]           = env->m_WarningTags[5];
  m_WarningTags[6].ti_Tag    = JPGTAG_TAG_DONE;
  //
  CleanWarnQueue();
  // Setup the pthread identifier
#ifdef PTHREAD_DEBUGGING
  SetSelfId();
#endif
#ifdef RECORD_CB_SCHEDULING
  m_iThreadId    = m_pParent->m_iNextAvailId++;
#endif
}
///

/// Environ::~Environ
Environ::~Environ(void)
{
  // Check whether this is a child environment. If so,
  // check whether it accumulated any warnings. If it did,
  // forward its warning now into the warning of the parent.
  // This is thread-safe because the threads are only allocated
  // and released within the master/supervisor thread.
  if (m_pParent) {
    // This CPU becomes now available again.
    if (m_WarnRoot.m_Exception.ErrorOf()) {
      m_pParent->m_WarnRoot.m_Exception = m_WarnRoot.m_Exception;
    }
    //
    // Merge the warnings.
    m_pParent->MergeWarningQueueFrom(this);
  }
  //
  // Check if this was a copy that was made for a side-thread.
  if (m_Root.m_pActive && m_pParent == NULL) {
    //
    //
#ifdef HIST
    PrintHist();
#endif
    //
    //
#if CHECK_LEVEL > 0
    // Check only on the final destruction.
    printf("\n%ld bytes memory not yet released.\n"
           "\n%ld bytes maximal required.\n"
           "\n%ld allocations performed.\n",
           long(totalmem),long(maxmem),long(malloccount));
# ifndef USE_VALGRIND
    // All memory released?
#  if defined(HAVE_ATOMIC_ADDSUB) || defined(USE_I386_XADD) || defined(USE_INTERLOCKED)
    assert(totalmem == 0);
#  else
    printf("\nFinal memory check skipped,\n"
           "unreliable due to missing locking primitive\n");
#  endif
# endif
#endif
  }
}
///

/// Environ::MergeWarningQueueFrom
// Merge the contents of our warning queue from the
// warning queue of the given environment.
void Environ::MergeWarningQueueFrom(class Environ *p)
{   
  int i;
  //
  // Merge the warnings.
  if (m_bSuppressMultiple) {
    for (i = 0;i < WarnQueueSize;i++) {
      if (!p->m_WarnQueue[i].isEmpty()) {
        // This will also enter the warning into the database.
        isWarned(p->m_WarnQueue[i]);
        p->m_WarnQueue[i].Reset();
      }
    }
  }
}
///

/// Environ::Throw
void Environ::Throw(const LONG error,const char *what,const LONG line, 
                    const char *where, 
                    const char *description)
{
  // Must have an error.
  assert(error);
  // Create an exception and forward to the thrower.
  Throw(Exception(error,what,line,where,description));
}
//
///

/// Environ::Throw
void Environ::Throw(const class Exception &exc)
{
  class ExceptionStack *es = m_Root.m_pActive;
  //
  assert(es);
  assert(es->m_pPrevious);
  assert(es->m_pRoot == &m_Root);
  //
  // There must be a reason code. Internal errors are obsolete.
  assert(exc.ReasonOf());
  //
  m_Root.m_Exception    = exc;
  es->Unlink();
  //
  // Deliver this message to the exception hook
  // FIX: No, don't. Have to do this manually in the outermost catch
  /*
    ForwardMessage(m_pExceptionHook,m_ExceptionTags,
    error,what,line,where,description);
  */
  //
  // And now jump to the (hopefully collected) catch line.
  longjmp(es->m_JumpDestination,1);
}
//
///

/// Environ::ReThrow
// Throw the last active exception again. This emulates a "throw;" in C++
void Environ::ReThrow(void)
{
  class ExceptionStack *es = m_Root.m_pActive;
  //
  assert(es);
  assert(es->m_pPrevious);
  assert(m_Root.m_Exception.ErrorOf());
  //
  es->Unlink();
  // The error was released to the hook already, hence we need not
  // to forward it again.
  longjmp(es->m_JumpDestination,1);
}
///

/// Environ::CleanWarnQueue
// Clean the warn queue/history, thus report all warnings from this
// point on again.
void Environ::CleanWarnQueue(void)
{
  int i;

  for(i = 0;i < WarnQueueSize;i++) {
    m_WarnQueue[i].Reset();
  }
}
///  

/// Environ::isWarned
// Check whether the given warning (at the line and source file) is already
// in the warning database. In case it is, return false. Otherwise, enter
// it to the data base and return true.
bool Environ::isWarned(const class Exception &e)
{
  int i;
  
  // This algorithm here implements a simple move-to-front list
  // where entries that re-arrive in the list bubble to the front, and
  // new entries replace old at the end of the list. This should be
  // sufficient.
  //
  for(i = 0;i < WarnQueueSize;i++) {
    if (m_WarnQueue[i].isEmpty()) {
      // Reached end of the queue, has not been warned about this entry.
      // Now move the new entry to the front, placing the old front
      // entry to here.
      break;
    } else {
      // is already in the database? Note that the file name is used
      // here as a key. No string comparison! This is good enough
      // for our purposes.
      if (m_WarnQueue[i] == e) {
        // Found already. Ok, move this to front.
        if (i > 0) {
          m_WarnQueue[i]   = m_WarnQueue[i-1];
          m_WarnQueue[i-1] = e;
        }
        return true;
      }
    }
  }
  //
  // Here the warning is not yet in the queue and should either
  // replace the entry at offset i, which is free, or a new
  // entry must be allocated at the end of the list.
  if (i >= WarnQueueSize)
    i = WarnQueueSize - 1;
  m_WarnQueue[i] = e;
  return false;
}
///

/// Environ::LowerToWarning
// Deliver the last caugt exception as warning only.
void Environ::LowerToWarning(void)
{
  // Just grab the data from the exception root and place it into
  // the warn root.
  Warn(m_Root.m_Exception);
}
///

/// Environ::Warn
void Environ::Warn(const LONG error,const char *what,const LONG line, 
                   const char *where, const char *description)
{  
  // Only if there is an error
  if (error) {
    Warn(Exception(error,what,line,where,description));
  }
}
///

/// Environ::Warn
void Environ::Warn(const class Exception &exc)
{  
  m_WarnRoot.m_Exception    = exc;
  //
  // Is the user warned about this exception already? If so,
  // do not do this again.
  if (!m_bSuppressMultiple || !isWarned(exc)) {
    // Deliver this message to the exception hook
    ForwardMessage(m_pWarningHook,m_WarningTags,m_WarnRoot.m_Exception);
    //
    // On debugging, print out the warning immediately
    PrintWarning();
  }
}
///

/// Environ::PostLastError
// Deliver the last error again over the exception hook
void Environ::PostLastError(void)
{
  ForwardMessage(m_pExceptionHook,m_ExceptionTags,
                 m_Root.m_Exception);
}
///

/// Environ::PostLastWarning
// Deliver the last warning again over the warning hook
void Environ::PostLastWarning(void)
{
  ForwardMessage(m_pWarningHook,m_WarningTags,
                 m_WarnRoot.m_Exception);
}
///

/// Environ::NextWarning
// In case several warnings are buffered, advance to the next buffered
// warning in this environment. Requires JPGTAG_EXC_SUPPRESS_IDENTICAL
// to be set.
void Environ::NextWarning(void)
{
  if (m_bSuppressMultiple) {
    int i,next = -1;
    // Deactivate the current warning by going thru the warning queue.
    for(i = 0;i < WarnQueueSize;i++) {
      if (!m_WarnQueue[i].isEmpty()) {
        if (m_WarnQueue[i] == m_WarnRoot.LastException()) {
          m_WarnQueue[i].Reset();
        } else {
          next = i; // This will be the next warning.
        }
      }
    }
    if (next >= 0) {
      m_WarnRoot.m_Exception = m_WarnQueue[next];
    } else {
      m_WarnRoot.m_Exception.Reset();
    }
  }
}
///

/// Environ::CoreAllocMem
inline void *Environ::CoreAllocMem(ULONG bytesize,ULONG reqments)
{
  // This is only thread-safe only if the user supplied
  // allocation hook is thread-safe. The HIST option is not,
  // thus don't do that.
  if (bytesize == 0) {
    return NULL;
  } else {
    void *mem;
    //
#if defined(HIST)
    RecordMemAlloc(bytesize);
#endif
    //
#ifdef MUNGE_MEM
    bytesize += 2 * sizeof(Align);
#endif
    //
    if (m_pAllocationHook) {
      // Fill in the tags by hand. This must be rather fast, so we
      // do it the nasty way.
      m_AllocationTags[0].ti_Data.ti_lData = bytesize;
      m_AllocationTags[1].ti_Data.ti_lData = reqments;
      mem = m_pAllocationHook->CallAPtr(m_AllocationTags);
    } else {
#ifdef HAVE_MALLOC
      mem = malloc(bytesize);
#if CHECK_LEVEL > 0
      malloccount++;
#endif
#else
      mem = NULL;
#endif
    }
    // In case no memory is here, throw an exception.
    // Except for debugging....
    assert(mem);
    if (mem == NULL) {
      class Environ *m_pEnviron = this; // for the macro.
      JPG_THROW(OUT_OF_MEMORY,"Environ::AllocMem","Out of free memory, aborted");
    }
    //
#ifdef MUNGE_MEM
    totalmem += bytesize; 
    if (totalmem > maxmem)
      maxmem = totalmem;
    {
      ULONG  s = bytesize;
      ULONG *p = (ULONG *)mem;
      while(s >= sizeof(ULONG)) {
        *p++ = 0xdeadf00dUL;
        s   -= sizeof(ULONG);
      }
      UBYTE *q = (UBYTE *)p;
      switch(s) {
      case 3:
        *q++ = 0xde;
        /* fall through */
      case 2:
        *q++ = 0xad;
        /* fall through */
      case 1:
        *q++ = 0xf0;
      }
    }
    {
      void *mmem              = mem;
      *(ULONG *)mmem          = bytesize;
      mmem                    = (void *)(((Align *)mmem)+1);
      *(class Environ **)mmem = this;
      mem                     = (void *)(((Align *)mem)+2);
    }    
#endif
    return mem;
  }
}
///

/// Environ::CoreFreeMem
// Free a memory block
inline void Environ::CoreFreeMem(void *mem,ULONG bytesize)
{
  // This is only thread-safe only if the user supplied
  // allocation hook is thread-safe. The HIST option is not,
  // thus don't do that.
  if (mem) {
#ifdef MUNGE_MEM
    mem       = (void *)(((Align *)mem)-2);
    bytesize += 2*sizeof(Align);
    //
    // Allocation and release size match?
    assert(bytesize == *(ULONG *)mem);
    totalmem -= bytesize;
    //
    // Munge memory again
    {
      ULONG  s = bytesize;
      ULONG *p = (ULONG *)mem;
      while(s >= sizeof(ULONG)) {
        *p++ = 0xdeadbeefUL;
        s   -= sizeof(ULONG);
      }
      UBYTE *q = (UBYTE *)p;
      switch(s) {
      case 3:
        *q++ = 0xde;
        /* fall through */
      case 2:
        *q++ = 0xad;
        /* fall through */
      case 1:
        *q++ = 0xbe;
      }
    }
#endif
    //
#ifdef HIST
    RecordMemFree(bytesize);
#endif
    //
    //
    if (m_pReleaseHook) {
      struct JPG_TagItem release[4];
      // Fill in the tags by hand. This must be rather fast, so we
      // do it the nasty way.
      release[0] = m_ReleaseTags[0];
      release[0].ti_Data.ti_lData = bytesize;
      release[1] = m_ReleaseTags[1];
      release[1].ti_Data.ti_pPtr  = mem;
      release[2] = m_ReleaseTags[2];
      release[3] = m_ReleaseTags[3];
      m_pReleaseHook->CallAPtr(release);
    } else {
#ifdef HAVE_FREE
      free(mem);
#else
      JPG_FATAL("Cannot release memory, no free function and no release hook");
#endif
    }
  }
}
///

/// Environ::AllocVec
void *Environ::AllocVec(size_t bytesize,ULONG requirements)
{
  size_t *mem;
  // This is build directly on AllocMem
  bytesize += sizeof(union Align);
  mem       = (size_t *)CoreAllocMem(ULONG(bytesize),requirements);
  *mem      = bytesize; // enter the bytesize
  return (void *)(ptrdiff_t(mem) + sizeof(union Align));
}
///

/// Environ::AllocVec without reqments
void *Environ::AllocVec(size_t bytesize)
{
  size_t *mem;
  // This is build directly on AllocMem
  bytesize += sizeof(union Align);
  mem       = (size_t *)CoreAllocMem(ULONG(bytesize),0);
  *mem      = bytesize; // enter the bytesize
  return (void *)(ptrdiff_t(mem) + sizeof(union Align)); 
}
///

/// Environ::AllocMem
void *Environ::AllocMem(size_t bytesize,ULONG reqments)
{
  return CoreAllocMem(ULONG(bytesize),reqments);
}
///

/// Environ::AllocMem without reqments
void *Environ::AllocMem(size_t bytesize)
{
  return CoreAllocMem(ULONG(bytesize),0);
}
///

/// Environ::FreeVec
void Environ::FreeVec(void *mem)
{
  if (mem) {
    size_t *sptr = (size_t *)(ptrdiff_t(mem) - sizeof(union Align));
    CoreFreeMem(sptr,ULONG(*sptr));
  }
}
///

/// Environ::FreeMem
void Environ::FreeMem(void *mem,size_t bytesize)
{
  CoreFreeMem(mem,ULONG(bytesize));
}
///

/// Environ::ForwardMessage
// Forward a warning or an exception to the supplied hook
void Environ::ForwardMessage(struct JPG_Hook *hook,struct JPG_TagItem *tags,
                             const Exception &exc)
{
  // Check whether a hook is available. If not, ignore.
  // Note that this is only thread-safe if the user supplied hook is.
  if (hook) {
    // Fill in the tags the dirty way
    tags[0].ti_Data.ti_lData = exc.ErrorOf();
    tags[1].ti_Data.ti_pPtr  = const_cast<char *>(exc.ObjectOf());
    tags[2].ti_Data.ti_lData = exc.LineOf();
    tags[3].ti_Data.ti_pPtr  = const_cast<char *>(exc.SourceOf());
    tags[4].ti_Data.ti_pPtr  = const_cast<char *>(exc.ReasonOf());
    //
    //
    hook->CallAPtr(tags);
  }
}
///

/// Environ::GetInformation
// Get information about the environment.
void Environ::GetInformation(struct JPG_TagItem *tags) const
{
  struct JPG_TagItem *curtag;
  //
  // This can be only called from the root environment,
  // and is thus thread-safe
  //
  for (curtag = tags; curtag != NULL; curtag = curtag->NextTagItem()) {
    switch (curtag->ti_Tag) {
    case JPGTAG_MIO_ALLOC_HOOK:
      curtag->ti_Data.ti_pPtr  = m_pAllocationHook;
      curtag->SetTagSet();
      break;
    case JPGTAG_MIO_RELEASE_HOOK:
      curtag->ti_Data.ti_pPtr  = m_pReleaseHook;
      curtag->SetTagSet();
      break;
    case JPGTAG_MIO_KEEPSIZE:
      curtag->ti_Data.ti_lData = true; // always required now.
      curtag->SetTagSet();
      break;
    case JPGTAG_EXC_EXCEPTION_HOOK:
      curtag->ti_Data.ti_pPtr  = m_pExceptionHook;
      curtag->SetTagSet();
      break;
    case JPGTAG_EXC_WARNING_HOOK:
      curtag->ti_Data.ti_pPtr  = m_pWarningHook;
      curtag->SetTagSet();
      break;
    }
  }
}
///

/// Fatal
#if CHECK_LEVEL > 0
void Fatal(const char *msg,const char *file,int line)
{
  char *x = NULL;
  fprintf(stderr,"%s.\nFatal error at line %d, file %s, execution aborted.\n",msg,line,file);
  *x = 0;
}
#endif
///

/// global new operator is now obsolete, at least for the check level
#if CHECK_LEVEL > 0 && !defined(GLOBAL_NEW_IS_OK)
void *operator new(size_t)
{
  assert(false);
  return (void *)(ptrdiff_t(0xdeadf00dUL));
}

void *operator new[](size_t)
{
  assert(false);
  return (void *)(ptrdiff_t(0xdeadf00dUL));
}
#endif
///
