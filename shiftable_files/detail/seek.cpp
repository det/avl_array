///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  seek.cpp
  --------
  
  Implementation of seek (random access) primitives.
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;


/*  -------------------------------------------------------
    find_pos()

    Find the position (block, and position within it) that
    corresponds to an absolute position in the file. This
    requires a travel from the root, down to the searched
    node.
*/

void shiftable_file::find_pos (uint32_t pos,
                               uint32_t & cur_node,
                               uint32_t & rel_pos) const
{
  if (!is_open())
    return;

  if (pos==0)                            // Beginning
  {
    rel_pos = 0;
    cur_node = PDUMMY->m_next;
    return;
  }

  if (pos >= PDUMMY->m_bytes_subtree)    // End (or further)
  {
    rel_pos = pos - PDUMMY->m_bytes_subtree;
    cur_node = DUMMY;
    return;
  }

  uint32_t n, m, left, right;

    // Start with the root (remember that the dummy has no right
    // child, and the effective root is the dummy's left child

  n = PDUMMY->child (L);

      // For every subtree, the position (in it) of its root
      // node is equal to the sum of bytes of its left subtree

  for (;;)            // Travel down updating pos to make it the
  {                   // index in the visited subtree, until it
    shf_assert (n>0); // fits the index of the subtree's root

    m = LEFT(n);
    left = m ? PNODE(m)->m_bytes_subtree : 0;
    right = left + PNODE(n)->m_bytes;

    if (pos<left)            // A step down-left doesn't
    {                        // touch pos, while a step
      n = m;                 // down-right decreases pos
    }                        // by the sum of bytes of
    else if (pos<right)      // the nodes we leave at the
    {                        // left side (including the
      rel_pos = pos - left;  // subtree's root)
      cur_node = n;
      return;
    }
    else
    {
      pos -= right;
      n = RIGHT(n);
    }
  }
}


/*  -------------------------------------------------------
    seek_set()

    Jump to the byte at position 'pos' of the payload data.
    This requires finding the block that contains the specified
    position, and then updating m_cur_node, m_abs_pos and
    m_rel_pos.

    This method is optimized for:

            - jumps to the end (or after the end)   O(1)
            - jumps to the beginning                O(1)
            - small jumps                           O(log N) (*)

    (*): where N is the size of the jump (and _not_ the
                                          size of the file)
*/

void shiftable_file::seek_set (uint32_t pos)
{
  uint32_t n;

  if (pos >= PDUMMY->m_bytes_subtree)   // If pos >= end
  {
    n = PREV(DUMMY);

    if (pos == PDUMMY->m_bytes_subtree &&
        PNODE(n)->m_bytes < BLOCK_SIZE)
    {
      m_cur_node = n;
      m_rel_pos = PNODE(n)->m_bytes;
    }
    else
    {
      m_cur_node = DUMMY;
      m_rel_pos = pos - PDUMMY->m_bytes_subtree;
    }

    m_abs_pos = pos;
    return;
  }

  if (pos <= PNEXT(DUMMY)->m_bytes)   // If pos near beginning
  {
    m_cur_node = NEXT(DUMMY);
    m_rel_pos = m_abs_pos = pos;
    return;
  }

         // Otherwise... move through the tree to the target
         // position. Choose at every step, between the available
         // links (parent, left/right children, previous/next),
         // the one that leads to the best position (the one
         // nearest to the target, in bytes), even if this
         // implies overpassing it.

  uint32_t best, offset, cur, distmin, dist, start, end;

  for (;;)
  {
    if (pos < m_abs_pos)       // Move towards the beginning...
    {
      if (pos >= m_abs_pos-m_rel_pos)
      {
        m_rel_pos -= m_abs_pos-pos;    // Target pos is inside
        m_abs_pos = pos;               // the current block
        return;
      }

      m_abs_pos -= m_rel_pos;
      m_rel_pos = 0;

      best = PREV(m_cur_node);   // 1st. candidate: link to prev

      if (m_abs_pos-pos <= PNODE(best)->m_bytes) // Target inside
      {                                          // prev. block
        m_cur_node = best;
        m_rel_pos = PNODE(best)->m_bytes - (m_abs_pos-pos);
        m_abs_pos = pos;
        return;
      }

      distmin = m_abs_pos - pos - PNODE(best)->m_bytes;
      offset = PNODE(best)->m_bytes;

      cur = LEFT(m_cur_node);    // 2nd. candidate: left child
                                 // (if any, and not the prev.)
      if (cur &&
          cur != PREV(m_cur_node))
      {
        end = m_abs_pos - PRIGHT(cur)->m_bytes_subtree;
        start = end - PNODE(cur)->m_bytes;

        if (pos<start)
          dist = start - pos;
        else if (pos>end)
          dist = pos - end;
        else
        {
          m_cur_node = cur;
          m_rel_pos = pos - start;   // Target inside
          m_abs_pos = pos;           // left child
          return;
        }

        if (dist<distmin)            // Better candidate?
        {
          best = cur;
          distmin = dist;
          offset = m_abs_pos - start;
        }
      }

      cur = PARENT(m_cur_node);      // 3rd. candidate: parent

      if (RIGHT(cur) == m_cur_node &&
          cur != PREV(m_cur_node))
      {
        end = m_abs_pos -
              PLEFT(m_cur_node)->m_bytes_subtree;
        start = end - PNODE(cur)->m_bytes;

        if (pos<start)
          dist = start - pos;
        else if (pos>end)
          dist = pos - end;
        else
        {
          m_cur_node = cur;
          m_rel_pos = pos - start;   // Target inside
          m_abs_pos = pos;           // parent
          return;
        }

        if (dist<distmin)            // Better candidate?
        {
          best = cur;
          distmin = dist;
          offset = m_abs_pos - start;
        }
      }

      m_cur_node = best;       // Go to best candidate
      m_abs_pos -= offset;
    }
    else if (pos > m_abs_pos)       // Move towards the end...
    {
      if (pos <= m_abs_pos-m_rel_pos+
                 PNODE(m_cur_node)->m_bytes)
      {
        m_rel_pos += pos-m_abs_pos;    // Target pos is inside
        m_abs_pos = pos;               // the current block
        return;
      }

      m_abs_pos -= m_rel_pos;
      m_rel_pos = 0;

      best = NEXT(m_cur_node);   // 1st. candidate: link to next

      if (pos-m_abs_pos <= PNODE(best)->m_bytes+
                           PNODE(m_cur_node)->m_bytes)
      {
        m_rel_pos = pos - m_abs_pos -            // Target inside
                    PNODE(m_cur_node)->m_bytes;  // next block
        m_cur_node = best;
        m_abs_pos = pos;
        return;
      }

      offset = PNODE(m_cur_node)->m_bytes;
      distmin = pos-m_abs_pos -
                PNODE(best)->m_bytes - offset;

      cur = RIGHT(m_cur_node);    // 2nd. candidate: right child
                                  // (if any, and not the next)
      if (cur &&
          cur != NEXT(m_cur_node))
      {
        start = m_abs_pos + PNODE(m_cur_node)->m_bytes +
                PLEFT(cur)->m_bytes_subtree;
        end = start + PNODE(cur)->m_bytes;

        if (pos<start)
          dist = start - pos;
        else if (pos>end)
          dist = pos - end;
        else
        {
          m_cur_node = cur;
          m_rel_pos = pos - start;   // Target inside
          m_abs_pos = pos;           // right child
          return;
        }

        if (dist<distmin)            // Better candidate?
        {
          best = cur;
          distmin = dist;
          offset = start - m_abs_pos;
        }
      }

      cur = PARENT(m_cur_node);      // 3rd. candidate: parent

      if (LEFT(cur) == m_cur_node &&
          cur != NEXT(m_cur_node))
      {
        start = m_abs_pos + PNODE(m_cur_node)->m_bytes +
                PRIGHT(m_cur_node)->m_bytes_subtree;
        end = start + PNODE(cur)->m_bytes;

        if (pos<start)
          dist = start - pos;
        else if (pos>end)
          dist = pos - end;
        else
        {
          m_cur_node = cur;
          m_rel_pos = pos - start;   // Target inside
          m_abs_pos = pos;           // parent
          return;
        }

        if (dist<distmin)            // Better candidate?
        {
          best = cur;
          distmin = dist;
          offset = start - m_abs_pos;
        }
      }

      m_cur_node = best;       // Go to best candidate
      m_abs_pos += offset;
    }
    else           // Neither lesser nor greater...
      return;      // ...already in the target position!
  }
}
