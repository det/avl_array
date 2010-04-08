///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
    alloc_free.cpp
    --------------
    
    Allocation/deallocation of nodes for the tree.
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;

/*  -------------------------------------------------------
    free_nodes_contiguous()

    Deallocate 'num' contiguous nodes starting at the
    position of node 'first'.
*/

void shiftable_file::free_nodes_contiguous (uint32_t first,
                                            uint32_t num)
{
  uint32_t i, last;
  header * h;

  if (num<1)
    return;

  shf_assert (first>DUMMY);

  h = PHEADER;
  last = first + num - 1;           // Last node (included)

  PNODE(first)->free ();            // Insert at beginning
  PNODE(first)->prev_free() = 0;    // for efficient
                                    // usage of mem. pages
  for (i=first; i<last; i++)
  {
    PNODE(i)->next_free() = i+1;
    PNODE(i+1)->free ();
    PNODE(i+1)->prev_free() = i;
  }

  PNODE(last)->next_free() = h->m_free_list_first;

  if (h->m_free_list_first!=0)  // If free list is _not_ empty
  {
    shf_assert (h->m_free_count>0 && h->m_free_list_last!=0);
    shf_assert (PNODE(h->m_free_list_first)->prev_free()==0);
    shf_assert (PNODE(h->m_free_list_last)->next_free()==0);

    PNODE(h->m_free_list_first)->prev_free() = last;
  }
  else                          // otherwise, (it _is_ empty)
  {
    shf_assert (h->m_free_count==0 && h->m_free_list_last==0);

    h->m_free_list_last = last;
  }

  h->m_free_list_first = first;
  h->m_free_count += num;
}

/*  -------------------------------------------------------
    free_nodes_list()

    Deallocate 'num' nodes starting at 'first' and
    following the linked list ('m_next' field).
*/

void shiftable_file::free_nodes_list (uint32_t first,
                                      uint32_t num)
{
  uint32_t i, last, next, prev, cur;
  header * h;
  node * p;

  if (num<1)
    return;

  shf_assert (first>DUMMY);

  h = PHEADER;

  if (num==1)        // Just one node...
  {
    cur = 0;
    last = first;
  }
  else               // More than one...
  {
    cur = first;              // Prepare the first one
    p = PNODE(first);
    next = p->m_next;
    p->free ();               // Reset
    p->prev_free() = 0;
    p->next_free() = next;    // Link

    for (i=1; i<num-1; i++)
    {
      prev = cur;             // Prepare the rest, excepting
      cur = next;             // the las one

      shf_assert (cur>DUMMY);

      p = PNODE(cur);
      next = p->m_next;

      p->free ();             // Reset
      p->prev_free() = prev;
      p->next_free() = next;  // Link
    }

    last = next;
  }

  shf_assert (last>DUMMY);
                              // Finally, the last node
  p = PNODE(last);
  p->free ();                             // Reset
  p->prev_free() = cur;
  p->next_free() = h->m_free_list_first;  // Link

  if (h->m_free_list_first!=0)  // If free list is _not_ empty
  {
    shf_assert (h->m_free_count>0 && h->m_free_list_last!=0);
    shf_assert (PNODE(h->m_free_list_first)->prev_free()==0);
    shf_assert (PNODE(h->m_free_list_last)->next_free()==0);

    PNODE(h->m_free_list_first)->prev_free() = last;
  }
  else                          // otherwise, (it _is_ empty)
  {
    shf_assert (h->m_free_count==0 && h->m_free_list_last==0);

    h->m_free_list_last = last;
  }

  h->m_free_list_first = first;
  h->m_free_count += num;
}

/*  -------------------------------------------------------
    alloc_nodes()

    Allocate 'num' nodes taking them out of the free nodes
    list. Return the number of the first one. Link them
    making a _single_ linked list via the 'm_next' field.
*/

uint32_t shiftable_file::alloc_nodes (uint32_t num)
{
  uint32_t i, first, last, next;
  header * h;
  node * p;

  if (num<=0)
    return 0;

  h = PHEADER;

  shf_assert (num<=h->m_free_count);
  shf_assert (h->m_free_list_first>DUMMY);

  next = first = h->m_free_list_first;
  p = PNODE(first);

  for (i=0; i<num-1; i++)     // Prepare all but the last one
  {
    shf_assert (p->is_free());
    shf_assert (p->next_free()>DUMMY);

    p->m_next = next = p->next_free();  // Grab next and link
    p = PNODE(next);                    // Advance
  }

  shf_assert (p->is_free());

  last = next;                // Grab last node
  next = p->next_free();      // Don't loose the remains
  p->m_next = 0;              // Terminate extracted list

  if (next!=0)                // If remaining list is _not_ empty
  {
    shf_assert (next>DUMMY);
    shf_assert (h->m_free_count>num);

    PNODE(next)->prev_free() = 0;
  }
  else                        // Otherwise (it _is_ empty now)
  {
    shf_assert (h->m_free_count==num);

    h->m_free_list_last = 0;
  }

  h->m_free_list_first = next;   // Link head of remaining list
  h->m_free_count -= num;

  return first;
}

/*  -------------------------------------------------------
    unfree_node()

    Allocate a specific node. Not any node, but the node
    at position 'pos'. (Simply take it out of the free
    nodes list).
*/

void shiftable_file::unfree_node (uint32_t pos)
{
  uint32_t prev, next;
  node * n;
  header * h;

  n = PNODE(pos);
  h = PHEADER;

  shf_assert (n->is_free());

  shf_assert (h->m_free_list_first &&
              h->m_free_list_last &&
              h->m_free_count);

  shf_assert ((n->prev_free()==0) == (h->m_free_list_first==pos));
  shf_assert ((n->next_free()==0) == (h->m_free_list_last==pos));

  shf_assert ((n->prev_free()==0 && n->next_free()==0) ==
              (h->m_free_count==1));

  prev = n->prev_free ();       // Get neighbors, if any
  next = n->next_free ();       // ('pos' might be the first,
                                // or the last, or none, or
  if (prev)                     // both first and last!)
  {
    PNODE(prev)->next_free() = next;
    n->prev_free() = 0;
  }                                    // Update previous node
  else                                 // or head pointer
    h->m_free_list_first = next;

  if (next)
  {
    PNODE(next)->prev_free() = prev;
    n->next_free() = 0;
  }                                    // Update next node
  else                                 // or tail pointer
    h->m_free_list_last = prev;

  h->m_free_count --;
}
