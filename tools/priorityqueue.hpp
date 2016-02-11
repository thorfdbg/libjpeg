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


