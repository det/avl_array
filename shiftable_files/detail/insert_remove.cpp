///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  insert_remove.cpp
  -----------------
  
  Implementation of insertion/removal primitives.
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;


/*  -------------------------------------------------------
    insert()

    Like write() but, instead of overwriting, it "shifts"
    the contents that follow the current position.
*/

uint32_t shiftable_file::insert (const void * buff,
                                 uint32_t bytes)
{
  uint32_t size, room;
  node * n;

  if (!is_open())
    return 0;

  size = PDUMMY->m_bytes_subtree;

  if (size+bytes < size ||        // >4GB...
      size+bytes < bytes ||       // ...overflow
      size+bytes > MAX_SIZE)
    return 0;

  if (m_abs_pos >= size)          // Inserting at the end
    return write (buff, bytes);   // is simply writing there

  check_integrity (true);

  shf_assert (m_cur_node>DUMMY);

  n = PNODE(m_cur_node);

  if (bytes < BLOCK_SIZE)      // Try the easiest case first
  {
    if (m_rel_pos == n->m_bytes &&
        n->m_next != DUMMY &&
        PNODE(n->m_next)->m_bytes < n->m_bytes)
    {
      m_cur_node = n->m_next;  // If placed in the boundary
      n = PNODE(m_cur_node);   // with next, and there is
      m_rel_pos = 0;           // more free space, move there
    }
    else if (m_rel_pos == 0 &&
             n->m_prev != DUMMY &&
             PNODE(n->m_prev)->m_bytes < n->m_bytes)
    {
      m_cur_node = n->m_prev;  // Same with previous
      n = PNODE(m_cur_node);
      m_rel_pos = n->m_bytes;
    }

    if (bytes <= BLOCK_SIZE-   // If the data fit in the
                 n->m_bytes)   // current block...
    {
      if (m_rel_pos != n->m_bytes)
        memmove (PBLOCK(m_cur_node) + m_rel_pos + bytes,
                 PBLOCK(m_cur_node) + m_rel_pos,
                 n->m_bytes - m_rel_pos);       // Make room

      memcpy (PBLOCK(m_cur_node) + m_rel_pos,   // Write
              buff, bytes);

      n->m_bytes += bytes;             // Propagate up
      update_counters (m_cur_node);    // size change

      m_rel_pos += bytes;              // Advance
      m_abs_pos += bytes;

      check_integrity (true);
      return bytes;
    }
  }

  room = make_room (m_cur_node, m_rel_pos);   // Second try

  const int8_t * pbuff;
  uint32_t pending;

  pbuff = (const int8_t*) buff;
  pending = bytes;

  if (room)       // While there is something to
  {               // insert and there is room for it...
    for (;;)
    {
      shf_assert (m_cur_node>DUMMY);
      shf_assert (PNODE(m_cur_node)->m_bytes<BLOCK_SIZE);

      n = PNODE(m_cur_node);
      size = BLOCK_SIZE - n->m_bytes;

      if (size>pending)
        size = pending;

      if (m_rel_pos != n->m_bytes)
        memmove (PBLOCK(m_cur_node) + m_rel_pos + size,
                 PBLOCK(m_cur_node) + m_rel_pos,
                 n->m_bytes - m_rel_pos);       // Make room

      memcpy (PBLOCK(m_cur_node) + m_rel_pos,   // Write
              pbuff, size);

      n->m_bytes += size;              // Propagate up
      update_counters (m_cur_node);    // size change

      m_rel_pos += size;
      m_abs_pos += size;
                                       // Advance
      pbuff += size;
      room -= size;
      pending -= size;

      if (!pending || !room ||
          m_rel_pos != n->m_bytes)
        break;

      m_cur_node = n->m_next;    // Go to the next node
      m_rel_pos = 0;             // and continue inserting
    }

    if (room>=BLOCK_SIZE)   // If make_room() freed more space
    {                       // than needed, and now we have empty
      uint32_t cur, next;   // blocks, take them out of the tree

      next = NEXT(m_cur_node);

      while (next>DUMMY && PNODE(next)->m_bytes==0)
      {
        cur = next;
        next = NEXT(cur);
        update_counters_and_rebalance (extract_node(cur));
        free_node (cur);
      }
    }
  }

  check_integrity (true);

  if (!pending)                  // Are we done?
    return bytes;                // Yes: good

  // We are not done... some new blocks must be allocated

  shf_assert (PNODE(m_cur_node)->m_bytes==BLOCK_SIZE);

  uint32_t num_new_blocks, cur_num_blocks, cur_used_blocks,
           i, first, last, cur, prev;
  header * h;

  num_new_blocks = ROUND_TO_POW2_MULT(pending,
                                      LOG2_BLOCK_SIZE) >>
                   LOG2_BLOCK_SIZE;
  h = PHEADER;
  cur_num_blocks = ( ( h->m_map_size -
                       h->m_meta_data_size ) >>
                     LOG2_BLOCK_SIZE            ) - 1;

  cur_used_blocks = cur_num_blocks - h->m_free_count;

  if (num_new_blocks > h->m_free_count) // If not enough
  {                                     // free blocks,
    h->set_current_op (OP_INSERT_GROW,  // enlarge
                       m_abs_pos,
                       bytes,
                       bytes-pending);

    if (!grow(cur_used_blocks+
              num_new_blocks))
      return bytes - pending;

    h = PHEADER;
    h->set_current_op (OP_NONE);

    find_pos (m_abs_pos, m_cur_node, m_rel_pos);

    cur_num_blocks = ( ( h->m_map_size -
                         h->m_meta_data_size ) >>
                       LOG2_BLOCK_SIZE            ) - 1;

    cur_used_blocks = cur_num_blocks - h->m_free_count;
  }

  shf_assert (num_new_blocks <= h->m_free_count);

  first = alloc_nodes (num_new_blocks); // Prepare list
  prev = m_cur_node;                    // of nodes to add

  for (i=0, cur=first; i<num_new_blocks-1; i++)
  {
    PNODE(cur)->m_bytes = BLOCK_SIZE;   // Assign sizes
    prev = cur;                         // (the data will
    cur = NEXT(cur);                    // be written
  }                                     // later)

  last = cur;        // Find last and link to its previous
  PREV(last) = prev;
  PNODE(last)->m_bytes = pending - (i << LOG2_BLOCK_SIZE);

      // If the insertion position breaks the current
      // block in two, the second part must be moved to
      // the end of the new blocks

  if (m_rel_pos != BLOCK_SIZE)
  {
    size = PNODE(m_cur_node)->m_bytes - m_rel_pos;

    if (PNODE(last)->m_bytes >= size)
      memcpy (PBLOCK(last) + PNODE(last)->m_bytes - size,
              PBLOCK(m_cur_node) + m_rel_pos,
              size);
    else               // Copy to last, or last and prev
    {
      memcpy (PBLOCK(last),
              PBLOCK(m_cur_node) + PNODE(m_cur_node)->m_bytes -
                                   PNODE(last)->m_bytes,
              PNODE(last)->m_bytes);

      size -= PNODE(last)->m_bytes;

      memmove (PBLOCK(PREV(last)) + BLOCK_SIZE - size,
               PBLOCK(m_cur_node) + m_rel_pos,
               size);
    }
  }
                                              // Build the
  if (worth_rebuild (num_new_blocks, false))  // tree from
  {                                           // scratch...
    NEXT(last) = NEXT(m_cur_node);
    NEXT(m_cur_node) = first;

    build_tree (PDUMMY->m_next, cur_used_blocks +
                                num_new_blocks);
  }
  else                                        // ... or...
  {
    uint32_t parent;        // ... insert the nodes one
    left_right side;        // by one (update counters and
                            // rebalance every time)
    NEXT(last) = 0;
    prev = m_cur_node;

    while (first)
    {
      cur = first;
      n = PNODE(cur);
      first = n->m_next;

      size = n->m_bytes;    // Initialize, but keep
      n->init();            // number of used bytes
      n->m_bytes = size;

      if (RIGHT(prev))          // If it has a right
      {                         // subtree, then the next
        parent = NEXT(prev);    // node (the leftmost node
        side = L;               // in this right subtree)
      }                         // has no left child. Put
      else                      // the new node there, as
      {                         // the left child of the
        parent = prev;          // next node
        side = R;
      }

      n->m_next = NEXT(prev);   // Insert the new node in
      PREV(n->m_next) = cur;    // the circular doubly
                                // linked list
      n->m_prev = prev;
      NEXT(prev) = cur;

      PNODE(parent)->child(side) = cur;    // Link parent
      n->m_parent = parent;

      update_counters_and_rebalance (cur);

      prev = cur;
    }
  }

  size = write (pbuff, pending);  // Now that the nodes have
  shf_assert (size==pending);     // been added, write the
                                  // data
  check_integrity (true);
  return bytes;
}


/*  -------------------------------------------------------
    remove()

    Take some data away, leaving the file as it these data
    had never been there. That is, "shift" the contents
    that follow the erased data, collapsing the deleted
    part to zero size.
*/

uint32_t shiftable_file::remove (uint32_t bytes)
{
  uint32_t size;

  if (!is_open() || bytes==0 || m_cur_node==DUMMY)
    return 0;

  check_integrity (true);
  size = PDUMMY->m_bytes_subtree;

  if (bytes > size - m_abs_pos)    // Can't remove more than
    bytes = size - m_abs_pos;      // what is there

  if (bytes==0)
    return 0;

  if (m_rel_pos ==                  // If current position is
      PNODE(m_cur_node)->m_bytes)   // a boundary (end of a
  {                                 // block, beg. of next),
    m_cur_node = NEXT(m_cur_node);  // advance to beginning
    m_rel_pos = 0;                  // of next
  }

  size = PNODE(m_cur_node)->m_bytes -  // Data after curr. pos.
         m_rel_pos;                    // in the current block
                                       // (easy to remove)
  if (bytes <= size)
  {                           // If this is the easy case...
    if (bytes < size)
      memmove (PBLOCK(m_cur_node) + m_rel_pos,
               PBLOCK(m_cur_node) + m_rel_pos + bytes,
               size - bytes);

    PNODE(m_cur_node)->m_bytes -= bytes;

    if (PNODE(m_cur_node)->m_bytes)
      update_counters (m_cur_node);
    else
    {
      uint32_t cur;

      cur = m_cur_node;
      m_cur_node = NEXT (m_cur_node);
      m_rel_pos = 0;

      update_counters_and_rebalance (extract_node(cur));
      free_node (cur);
    }
  }
  else       // Otherwise (removal affects later nodes too)...
  {
    uint32_t pending, num_del_nodes, cur_num_nodes;
    uint32_t cur, prev, next, end;
    header * h;

    PNODE(m_cur_node)->m_bytes -= size;  // First node is easy
    update_counters (m_cur_node);

    pending = bytes - size;        // Now go for the next ones

    if (m_rel_pos == 0)                // This time prefer end
    {                                  // of prev. in boundary
      m_cur_node = PREV(m_cur_node);               // position
      m_rel_pos = PNODE(m_cur_node)->m_bytes;
    }

    h = PHEADER;
    cur_num_nodes = ( (h->m_map_size -
                       h->m_meta_data_size) >>
                      LOG2_BLOCK_SIZE          ) - 1 -
                    h->m_free_count;

    prev = m_cur_node;
    cur = NEXT(m_cur_node);
    num_del_nodes = 0;
                                           // Count how many
    while (pending &&                      // nodes need to be
           pending >= PNODE(cur)->m_bytes) // deleted. Keep
    {                                      // track of previous
      num_del_nodes ++;                    // and next to them.
      pending -= PNODE(cur)->m_bytes;
      cur = NEXT(cur);
    }

    next = cur;

    if (pending)                 // If required, remove some
    {                            // last bytes from next node
      memmove (PBLOCK(cur),
               PBLOCK(cur) + pending,
               PNODE(cur)->m_bytes - pending);

      PNODE(cur)->m_bytes -= pending;
      update_counters (cur);
    }                                        // Delete sublist
                                             // of nodes in the
    if (worth_rebuild (num_del_nodes, true)) // most efficient
    {                                        // way...
      cur = NEXT(prev);
      free_nodes_list (cur, num_del_nodes);     // Rebuild all
                                                // from scratch
      NEXT(prev) = next;
      build_tree (NEXT(DUMMY), cur_num_nodes-
                               num_del_nodes);
    }
    else
    {
      end = next;
      cur = NEXT(prev);       // Or delete nodes one by one
                              // (update counters and rebalance
      while (cur!=end)        // the tree every time)
      {
        next = NEXT(cur);

        update_counters_and_rebalance (extract_node(cur));
        free_node (cur);

        cur = next;
      }
    }

    find_pos (m_abs_pos, m_cur_node, m_rel_pos);
  }

      // At this point, the bytes have been removed.
      // The tree is correctly balanced and has all counters
      // up to date. There are no empty blocks.

      // BUT... If we leave it here, there is a chance for
      // the unused space to grow without control (up to the
      // absurdness of one single used byte per block).
      // If possible, merge some neigbour blocks.

  check_integrity (true);

  uint32_t room, next;
                                  // Tear data apart (perhaps
  room = make_room (m_cur_node,   // the current node gets
                    m_rel_pos);   // empty)

  if (room>=BLOCK_SIZE) // If at least one block can be freed..
  {
    shf_assert (m_cur_node!=DUMMY);

    while (m_rel_pos==0 &&            // Walk backwards until
           PREV(m_cur_node)!=DUMMY)   // the boundary, even if
    {                                 // prev node is full!
      m_cur_node = PREV(m_cur_node);
      m_rel_pos = PNODE(m_cur_node)->m_bytes;
    }

    for (;;)                    // While the current node can
    {                           // absorb the data of the next
      next = NEXT(m_cur_node);  // one (perhaps emty)...

      if (next==DUMMY ||
          PNODE(m_cur_node)->m_bytes +
          PNODE(next)->m_bytes         > BLOCK_SIZE)
        break;

      if (PNODE(next)->m_bytes)
      {
        memcpy (PBLOCK(m_cur_node) + PNODE(m_cur_node)->m_bytes,
                PBLOCK(next),
                PNODE(next)->m_bytes);    // Absorb and delete

        PNODE(m_cur_node)->m_bytes += PNODE(next)->m_bytes;
      }

      update_counters_and_rebalance (extract_node(next));
      free_node (next);
    }

    update_counters (m_cur_node);
  }

  PHEADER->set_current_op (OP_NORMAL_SHRINK);

  if (!shrink())    // Decide whether to shrink the file
    return bytes;   // (and, if yes, proceed)

  PHEADER->set_current_op (OP_NONE);

  check_integrity (true);
  return bytes;
}


/*  -------------------------------------------------------
    make_room()

    Move data away from a specified position in order to
    obtain some free space. Note that the parameters are
    passed by reference. They will be updated as data are
    moved.
*/

uint32_t shiftable_file::make_room (uint32_t & cur_node,
                                    uint32_t & rel_pos)
{
  uint32_t i, n, m, first, last, movable,
           room_cur, room_prev, room_next, offset;
  bool changed;
  node * p, * q;

  shf_assert (cur_node);

  if (cur_node==DUMMY)
  {
    shf_assert (rel_pos==0);

    cur_node = PREV(cur_node);
    rel_pos = PNODE(cur_node)->m_bytes;

    if (cur_node==DUMMY)
    {
      shf_assert (PDUMMY->m_bytes_subtree==0);
      return 0;
    }
  }

  changed = false;
  first = last = cur_node;
  p = PNODE(cur_node);
  room_cur = BLOCK_SIZE - p->m_bytes;
  room_prev = room_next = 0;

  shf_assert (rel_pos<=BLOCK_SIZE);

  movable = rel_pos;

  // First, try to move the data at the 'left' even
  // more to the 'left' (that is, towards the beginnig)

  for (i=0,                  n=PREV(cur_node);
       i<COMPACTION_STEPS && n!=DUMMY;
       i++,                  n=p->m_prev)
  {
    first = n;                  // Walk left counting free
    p = PNODE(n);               // and occuppied bytes
    movable += p->m_bytes;
    room_prev += BLOCK_SIZE - p->m_bytes;
  }

  if (room_prev>0)              // If something can be done,
  {                             // walk back right compacting
    n = first;                  // data to the left
    p = PNODE(n);
    movable -= p->m_bytes;

    while (n!=cur_node && movable>0)
    {
      if (p->m_bytes<BLOCK_SIZE)     // If room in block p...
      {
        m = p->m_next;
        q = PNODE(m);

        while (m!=cur_node && q->m_bytes==0)
        {
          m = q->m_next;             // ... find some data
          q = PNODE(m);              // to copy into p
        }

        shf_assert (q->m_bytes>0);

        i = movable < q->m_bytes ?
            movable : q->m_bytes;         // Bytes to copy
        i = i < BLOCK_SIZE-p->m_bytes ?
            i : BLOCK_SIZE-p->m_bytes;

        memcpy (PBLOCK(n)+p->m_bytes,
                PBLOCK(m), i);            // Copy

        p->m_bytes += i;

        if (q->m_bytes>i)
          memmove (PBLOCK(m),             // Rearrange
                   PBLOCK(m)+i,           // source block
                   q->m_bytes-i);

        if (m==cur_node)           // If cur_node changed,
          rel_pos -= i;            // update rel_pos

        q->m_bytes -= i;
        movable -= i;
        changed = true;
      }
      else  // If no room in block p...
      {
        n = p->m_next;    // ... advance
        p = PNODE(n);
        movable -= p->m_bytes;
      }
    }
  }

  p = PNODE(cur_node);
  movable = p->m_bytes-rel_pos;

  // Now, try to move the data at the 'right' even
  // more to the 'right' (that is, towards the end)

  for (i=0,                  n=NEXT(cur_node);
       i<COMPACTION_STEPS && n!=DUMMY;
       i++,                  n=p->m_next)
  {
    last = n;                   // Walk right counting free
    p = PNODE(n);               // and occuppied bytes
    movable += p->m_bytes;
    room_next += BLOCK_SIZE - p->m_bytes;
  }

  if (room_next>0)              // If something can be done,
  {                             // walk back left compacting
    n = last;                   // data to the right
    p = PNODE(n);
    movable -= p->m_bytes;
    offset = 0;

    while (n!=cur_node && movable>0)
    {
      if (p->m_bytes<BLOCK_SIZE)     // If room in block p...
      {
        m = p->m_prev;
        q = PNODE(m);

        while (m!=cur_node && q->m_bytes==0)
        {
          m = q->m_prev;             // ... find some data
          q = PNODE(m);              // to copy into p
        }

        shf_assert (q->m_bytes>0);

        if (offset==0)
        {
          offset = movable < BLOCK_SIZE-p->m_bytes ?
                   movable : BLOCK_SIZE-p->m_bytes;

          shf_assert (offset>0);          // Rearrange
                                          // destination block
          if (p->m_bytes>0)
            memmove (PBLOCK(n)+offset, PBLOCK(n),
                     p->m_bytes);
        }

        i = offset < q->m_bytes ?         // Bytes to copy
            offset : q->m_bytes;

        shf_assert (i<=movable);
        shf_assert (i>0);

        memcpy (PBLOCK(n)+(offset-i),             // Copy
                PBLOCK(m)+(q->m_bytes-i), i);

        p->m_bytes += i;
        q->m_bytes -= i;
        movable -= i;
        offset -= i;
        changed = true;
      }
      else  // If no room in block p...
      {
        shf_assert (offset==0);

        n = p->m_prev;    // ... advance
        p = PNODE(n);
        movable -= p->m_bytes;
      }
    }
  }

  if (changed)      // If any bytes have been moved,
    for (;;)        // update byte counts
    {
      update_counters (first);
      if (first==last) break;
      first = NEXT(first);
    }

                              // Update position
  if (rel_pos==BLOCK_SIZE &&
      NEXT(cur_node)!=DUMMY)
  {
    cur_node = NEXT(cur_node);
    rel_pos = 0;
  }
  else
    while (rel_pos==0 &&
           PREV(cur_node)!=DUMMY &&
           PPREV(cur_node)->m_bytes<BLOCK_SIZE)
    {
      cur_node = PREV(cur_node);
      rel_pos = PNODE(cur_node)->m_bytes;
    }

  return room_cur + room_prev + room_next;
}


/*  -------------------------------------------------------
    extract_node()

    Take a given node out of the tree, but don't free it.
    Depending on the circumstances, the caller might need to
    call update_counters() or even
    update_counters_and_rebalance(), passing them the node
    number returned by extract_node().
*/

uint32_t shiftable_file::extract_node (uint32_t e)
{
  int side;
  uint32_t b, s, o, cl, cr;
  node * pe, * pb, * ps, * po;

  shf_assert (e!=0);
  shf_assert (e!=DUMMY);

  pe = PNODE(e);

  o = pe->m_parent;
  po = PNODE(o);

  cl = LEFT(e) ? PNODE(pe->left())->m_bytes_subtree : 0;
  cr = RIGHT(e) ? PNODE(pe->right())->m_bytes_subtree : 0;

  if (!LEFT(e) || !RIGHT(e))  // If one subtree is empty (or both)
  {
    side = LEFT(e) ? L : R;   // Take the other subtree
                              // (take R if both are empty)
    s = pe->child(side);      // and bypass the victim node
    b = o;                    // linking the non-empty
                              // subtree directly to the
    if (s!=0)                 // parent of the victim node
    {
      ps = PNODE(s);          // It must hang from the same
      ps->m_parent = o;       // side the victim did
    }

    if (po->child(L)==e)      // The potentially unbalanced
      po->child(L) = s;       // branch is from the victim's
    else                      // parent and upwards
      po->child(R) = s;
  }                           // Ok, no subtree is empty, but
                                               // if there's
  else if (RIGHT(pe->child(L))==0 ||           // a hole in
           LEFT(pe->child(R))==0)              // the inner
  {                                            // places two
                              // levels under the victim node

    side = RIGHT(pe->child(L))==0 ? L : R; // Hole

    b = s = pe->child(side);  // Pot. unbal.: hole's parent
    pb = ps = PNODE(s);
    ps->m_parent = o;

    if (po->child(L)==e)      // Put the hole's parent in the
      po->child(L) = s;       // place of the victim, adopting
    else                      // the victim's parent and the
      po->child(R) = s;       // victim's other subtree

    ps->child(1-side) = pe->child(1-side);
    PARENT(ps->child(1-side)) = s;
  }
  else           // Well, no subtree is empty and both inner
  {              // places two levels under the victim are
    if (cl>cr)   // occupied, so both next and previous nodes
    {            // of the victim are down there
      side = L;
      s = pe->m_prev;
    }                  // Choose one of them (the one in
    else               // the most populated subtree), and
    {                  // put it in the place of the victim
      side = R;
      s = pe->m_next;
    }                  // Potentially unbalanced branch: from
                       // the subsitute's parent and upwards
    ps = PNODE(s);

    b = ps->m_parent;               // The substitute has no
    pb = PNODE(b);                  // child in one side,
    pb->child(1-side) =             // but it might have one
             ps->child(side);       // in the other side
                                    // The substitute's parent
    if (pb->child(1-side)!=0)         // must adopt it
      PARENT(pb->child(1-side)) = b;

    ps->child(L) = pe->child(L);    // The substitute
    PARENT(ps->child(L)) = s;       // adopts the
    ps->child(R) = pe->child(R);    // victim's children
    PARENT(ps->child(R)) = s;

    ps->m_parent = o;
                                // The victim's parent
    if (po->child(L)==e)        // adopts the substitute
      po->child(L) = s;
    else
      po->child(R) = s;
  }

  PREV(pe->m_next) = pe->m_prev;  // Bypass the victim in the
  NEXT(pe->m_prev) = pe->m_next;  // circ. doubly linked list

  return b;  // Potentially unbalanced branch
             // (from b and upwards until the root)
}

