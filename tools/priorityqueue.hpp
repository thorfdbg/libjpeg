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
**
** PriorityQueue: This is a base object that allows byte indexed
** priority sorted queues, as for example used for the buffers of
** the generic box extension layer.
** 
** $Id: priorityqueue.hpp,v 1.6 2014/09/30 08:33:18 thor Exp $
**
*/

#ifndef TOOLS_PRIORITYQUEUE_HPP
#define TOOLS_PRIORITYQUEUE_HPP

/// Includes
#include "tools/environment.hpp"
///

/// Class PriorityQueue
// This object keeps a priority sorted list of something.
// Must derive from this object to have something useful.
template<class T>
class PriorityQueue : public JObject {
  //
protected:
  // Pointer to the next entry of the queue, or NULL
  // for the last one.
  T                   *m_pNext;
  //
  // Priority of this object
  ULONG                m_ulPrior;
  //
public:
  // Append the priorityQueue to an existing queue.
  PriorityQueue(T *&head,ULONG prior)
    : m_ulPrior(prior)
  {
    T **next = &head;
    //
    // Check whether we have a next entry. If so, insert here if our index is smaller
    // than that of the next item.
    while(*next && (*next)->m_ulPrior <= prior) {
      next = &((*next)->m_pNext);
    }
    // At this point, either next no longer exists, or its prior is larger than ours
    // and we must insert in front of it.
    m_pNext = *next;
    *next   = (T *)this;
  }
  //
  // Return the next element of this list.
  T *NextOf(void) const
  {
    return m_pNext;
  }
  //
  // Return the priority of this object.
  ULONG PriorOf(void) const
  {
    return m_ulPrior;
  }
  //
  // Remove this object from the priority queue given the head.
  void Remove(T *&head) const
  {
    const T *me = (const T *)this;
    T    **prev = &head;
    //
    while(*prev != me) {
      // if prev is NULL now, then the current node is not
      // in this list. How that?
      assert(*prev);
      prev = &((*prev)->m_pNext);
    }
    *prev  = m_pNext;
  }
  //
  // Attach another priority queue at the end of this queue, empty
  // the other queue.
  static void AttachQueue(T *&head,T *&other)
  {
    T **prev = &head;
    // 
    // First find the end of our buffer list.
    while(*prev)
      prev = &((*prev)->m_pNext);
    //
    *prev = other;
    other = NULL;
  }
  //
  // Find the last node of the given priority, or return NULL if there
  // is no such node. Note that the priority must match exactly here.
  static T *FindPriorityTail(T *head,ULONG prior)
  {
    T *next;

    while(head) {
      next = head->m_pNext;
      if (head->m_ulPrior == prior) {
        // A potential candiate for the node. Make this
        // a real candidate if the next node is absent
        // or has a higher priority.
        if (next == NULL || next->m_ulPrior > prior)
          return head;
      } else if (head->m_ulPrior > prior) {
        // Iterated too long, no candidate.
        break;
      }
      head = next;
    }
    return NULL;
  }
};
///

///
#endif


