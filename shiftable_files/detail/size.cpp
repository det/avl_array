///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  size.cpp
  --------
  
  Implementation of size-related primitives (get/change the size
  of the payload data or the size of the underlying true file).
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"
#include "../osal/osal.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;


/*  -------------------------------------------------------
    size()

    Return current payload data size.
*/

uint32_t shiftable_file::size () const
{
  return is_open() ? PDUMMY->m_bytes_subtree : 0;
}


/*  -------------------------------------------------------
    resize()

    Change payload data size (truncate, or add zeros). Call
    shrink() or grow() if required. Return true if
    everything is OK.

    NOTE: If resize() returns false, the resulting state
          is undetermined (i.e., memory map failed).

    // TODO: Fix resize() for safe error handling
*/

bool shiftable_file::resize (uint32_t new_size)
{
  uint32_t old_size;

  if (!is_open())
    return false;

  check_integrity (true);
  old_size = PDUMMY->m_bytes_subtree;

  if (new_size > old_size)              // Enlarge...
  {
    uint32_t first, last, bytes, room, next, i,
             num_new_blocks;
    node * n, * plast;

    bytes = new_size - old_size;     // Bytes to add
    last = PDUMMY->m_prev;           // Last node
    plast = PNODE(last);

    if (last!=DUMMY)  // If there is something in the tree...
    {
      if (bytes <= BLOCK_SIZE -      // If the new bytes fit
                   plast->m_bytes)   // in the last block...
      {
        memset (PBLOCK(last)+plast->m_bytes, 0, bytes);
        plast->m_bytes += bytes;
        update_counters (last);

        if (m_abs_pos >= old_size)
          find_pos (m_abs_pos, m_cur_node, m_rel_pos);

        check_integrity (true);
        return true;
      }
      else  // Otherwise, try moving data across neighbours
      {
        uint32_t cur_node, rel_pos;

        cur_node = last;
        rel_pos = plast->m_bytes;
        room = make_room (cur_node, rel_pos);  // Won't try to
        bytes = bytes < room ? bytes : room;   // add more bytes
                                               // than the space
        if (bytes>0)                           // freed (by now)
        {
          last = cur_node;        // Go to the end
          plast = PNODE(last);    // of the data

          do                      // Walk forward adding bytes
          {
            shf_assert (last!=DUMMY);

            i = bytes < BLOCK_SIZE-plast->m_bytes ?
                bytes : BLOCK_SIZE-plast->m_bytes;

            memset (PBLOCK(last)+plast->m_bytes, 0, i);
            plast->m_bytes += i;
            bytes -= i;
            update_counters (last);

            last = plast->m_next;
            plast = PNODE(last);
          }
          while (bytes>0);
                                // If make_room() freed more space
          while (last!=DUMMY)   // than needed, and now we have empty
          {                     // blocks, take them out of the tree
            next = NEXT(last);
            update_counters_and_rebalance (extract_node(last));
            free_node (last);
            last = next;
          }

          if (PDUMMY->m_bytes_subtree == new_size)  // If done...
          {
            find_pos (m_abs_pos, m_cur_node, m_rel_pos);
            check_integrity (true);
            return true;
          }
        }
      }
    }

    // At this point, it is possible that some bytes have
    // been added. Anyway, it was not enough. One or more
    // new blocks are required.

    num_new_blocks = ( new_size -
                       PDUMMY->m_bytes_subtree +
                       BLOCK_SIZE-1 )
                                       >> LOG2_BLOCK_SIZE;

    if (num_new_blocks >         // If the free blocks are
        PHEADER->m_free_count)   // not enough...
    {
      uint32_t cur_num_blocks, total;
      header * h;

      h = PHEADER;
      cur_num_blocks = ( (h->m_map_size -
                          h->m_meta_data_size) >>
                         LOG2_BLOCK_SIZE          ) - 1;

      total = cur_num_blocks +
              num_new_blocks - PHEADER->m_free_count;

      h->set_current_op (OP_RESIZE_GROW,
                         old_size,
                         new_size - old_size,
                         PDUMMY->m_bytes_subtree - old_size);

      if (!grow(total))
        return false;

      h = PHEADER;
      h->set_current_op (OP_NONE);
    }

    // Prepare new nodes (and their blocks)

    next = first = alloc_nodes (num_new_blocks);
    bytes = new_size - PDUMMY->m_bytes_subtree;

    while (bytes>BLOCK_SIZE)
    {
      shf_assert (next);

      memset (PBLOCK(next), 0, BLOCK_SIZE);
      n = PNODE(next);
      next = n->m_next;
      n->init ();
      n->m_next = next;
      n->m_bytes = BLOCK_SIZE;
      bytes -= BLOCK_SIZE;
    }

    shf_assert (bytes);
    shf_assert (next);
    shf_assert (NEXT(next) == 0);

    memset (PBLOCK(next), 0, BLOCK_SIZE);
    n = PNODE(next);
    n->init ();
    n->m_bytes = bytes;

    // Are the new nodes worth rebuild the whole tree?

    if (worth_rebuild(num_new_blocks,false))
    {
      // Yes: append the new nodes to the list, and
      //      then rebuild the whole tree

      header * h;
      uint32_t num_blocks;

      last = PDUMMY->m_prev;
      h = PHEADER;
      num_blocks = ( (h->m_map_size -
                      h->m_meta_data_size) >>
                     LOG2_BLOCK_SIZE          ) -
                   (1 + h->m_free_count);

      NEXT(last) = first;
      build_tree (PDUMMY->m_next, num_blocks);
    }
    else
    {
      // No: add nodes one by one, and rebalance every time

      do
      {
        next = first;
        n = PNODE(next);
        first = n->m_next;

        n->m_next = DUMMY;
        n->m_prev = n->m_parent = PDUMMY->m_prev;

        PREV(n->m_next) = next;
        NEXT(n->m_prev) = next;

        if (n->m_parent==DUMMY)
          PDUMMY->child(L) = next;
        else
          RIGHT(n->m_parent) = next;

        update_counters_and_rebalance (next);
      }
      while (first);
    }

    find_pos (m_abs_pos, m_cur_node, m_rel_pos);
    check_integrity (true);
    return true;
  }
  else if (new_size < old_size)          // Shrink...
  {
    uint32_t abs_pos, bytes;   // Simulate remove
                               // operation near the end
    abs_pos = m_abs_pos;
    m_abs_pos = new_size;
    find_pos (m_abs_pos, m_cur_node, m_rel_pos);

    bytes = remove (old_size - new_size);
    shf_assert (bytes==old_size-new_size);

    m_abs_pos = abs_pos;
    find_pos (m_abs_pos, m_cur_node, m_rel_pos);

    check_integrity (true);
    return true;
  }
  else return true; // new size == old size
}


/*  -------------------------------------------------------
    grow()

    Enlarge the file to a size that can contain a number of
    ocucuppied data blocks (num_blocks), plus some free
    nodes (see macro EXTRA_GROWTH). Returns true if
    everything is OK, and false if the requested size is
    impossible or a map operation fails.
*/

bool shiftable_file::grow (uint32_t num_blocks)
{
  uint32_t data_size, map_size, meta_data_size;
  uint32_t cur_num_blocks, next_new;
  header * h;

  check_integrity ();

  if (num_blocks>MAX_BLOCKS)  // If it's too much, give up
  {
    close (false);
    return false;
  }

  h = PHEADER;
  cur_num_blocks = ( (h->m_map_size -
                      h->m_meta_data_size) >>
                     LOG2_BLOCK_SIZE          ) - 1;

  if (num_blocks <= cur_num_blocks)
    return true;

  num_blocks = EXTRA_GROWTH (num_blocks); // Now we are at it,
                                          // ask for some more
  if (num_blocks>MAX_BLOCKS)
    num_blocks = MAX_BLOCKS;

  data_size = num_blocks << LOG2_BLOCK_SIZE;

  expanded_size (data_size,         // Compute new layout of
                 map_size,          // data and metadata
                 meta_data_size,
                 true);

  m_pfile->unmap ();
  m_pmap = NULL;

  if (!m_pfile->resize(map_size))   // TODO: error control
  {
    close (false);
    return false;
  }

  m_pmap = m_pfile->map ();

  if (!m_pmap)   // TODO: error control
  {
    close (false);
    return false;
  }

  h = PHEADER;  // m_pmap might have changed so...

  next_new = (h->m_map_size >>            // 1st new node
              LOG2_BLOCK_SIZE) - 1;

  if (next_new < (meta_data_size >> LOG2_BLOCK_SIZE))
    next_new = meta_data_size >> LOG2_BLOCK_SIZE;

  if (meta_data_size >     // If the metadata area grows, some
      h->m_meta_data_size) // nodes (and blocks) need to be moved
  {                        // from the beginning to the end (or
    uint32_t start, end;   // to any free position, if available)
    uint32_t bytes, i;
    uint32_t truly_free_blocks, min_bytes_move, min_blocks_move;
    bool can_copy_all_data, can_move_conflicting_blocks;

    bytes = PDUMMY->m_bytes_subtree;

    start = h->m_meta_data_size >> LOG2_BLOCK_SIZE; // Nodes to
    end = meta_data_size >> LOG2_BLOCK_SIZE;        // be moved

    if (end > (h->m_map_size >> LOG2_BLOCK_SIZE) - 1)
      end = (h->m_map_size >> LOG2_BLOCK_SIZE) - 1;

    min_bytes_move = min_blocks_move = 0;
    truly_free_blocks = h->m_free_count;

    for (i=start; i<end; i++)           // Count number of
      if (PNODE(i)->is_free())          // occupied nodes to
        truly_free_blocks --;           // move, total bytes
      else                              // to move, and
      {                                 // ex-free blocks
        min_blocks_move ++;
        min_bytes_move += PNODE(i)->m_bytes;
      }

    can_copy_all_data =
        map_size - h->m_map_size >= bytes;

    can_move_conflicting_blocks =
        num_blocks - cur_num_blocks +
        truly_free_blocks             >= min_blocks_move;

    shf_assert (can_copy_all_data ||
                can_move_conflicting_blocks);

    if (!can_move_conflicting_blocks || // If it's necessary, or
        (can_copy_all_data &&           // if all data need to be
         min_bytes_move==bytes))        // moved...
    {
      uint32_t size[2], pos[2], abs_pos, num;

      start = map_size - BLOCK_SIZE -
              ROUND_TO_POW2_MULT (bytes, LOG2_BLOCK_SIZE);

      abs_pos = m_abs_pos;           // Make a copy of the
      m_abs_pos = m_rel_pos = 0;     // current position
      m_cur_node = PDUMMY->m_next;
                                       // Read and write
      i = read (m_pmap+start, bytes);  // in the same file!
      shf_assert (i==bytes);           // (mem. mapped ;-)

      m_abs_pos = abs_pos;           // Restore position

      h->m_meta_data_size = meta_data_size;
      h->m_map_size = map_size;             // Change size

      size[0] = bytes;     size[1] = 0;
      pos[0] = start;      pos[1] = 0;

      num = make_list_of_nodes (size, pos);
      start >>= LOG2_BLOCK_SIZE;
                                    // Make a new tree for
      build_tree (start, num);      // the moved data
                                    // (now defragmented)
      end = start;
      start = meta_data_size >>     // Make a new free
              LOG2_BLOCK_SIZE;      // nodes list too

      h->m_free_list_first =
      h->m_free_list_last =
      h->m_free_count = 0;

      free_nodes_contiguous (start, end-start);

      check_integrity ();
      return true;
    }                   // If the growth is not so radical...

    for (i=start; i<end; i++)    // First, treat free nodes
      if (PNODE(i)->is_free())   // (just remove them from the
        unfree_node (i);         // free list)

    // Now, the nodes in the free list (if any) are really free

    for (i=start; i<end; i++)       // Now, move occupied nodes
      if (!(PNODE(i)->is_free()))   // to...
      {
        if (h->m_free_count)
          move_node (i, alloc_node());  // ...free positions (if
        else                            // possible), or...
        {
          memcpy (PBLOCK(next_new), PBLOCK(i), BLOCK_SIZE);
          move_node (i, next_new, false);
          PNODE(i)->init ();            // ...new nodes at the end
          next_new ++;                  // (in this case, move the
        }                               // block in first place!)
      }

    h->m_meta_data_size = meta_data_size;
  }

  h->m_map_size = map_size;
                                     // Free [remaining] new nodes
  free_nodes_contiguous (next_new,
                         (map_size>>LOG2_BLOCK_SIZE)-1-next_new);
  check_integrity ();
  return true;
}


/*  -------------------------------------------------------
    shrink()

    If num_blocks>0 (and the current number of blocks is
    greater), shrink to the specified number of blocks.
    If num_blocks==0 (the default value), decide whether to
    shrink or not (see macro WORTH_SHRINK) and, if yes, to
    what size (see macro EXTRA_GROWTH).
    Return true if everything is OK, and false if a map
    operation fails.
*/

bool shiftable_file::shrink (uint32_t num_blocks/*=0*/)
{
  uint32_t data_size, map_size, meta_data_size;
  uint32_t cur_num_blocks, num_used_blocks;
  uint32_t start_del, end_del, i;
  uint32_t num_moving, num_free;
  header * h;

  check_integrity ();

  if (m_disable_shrink)
    return true;

  h = PHEADER;
  cur_num_blocks = ( (h->m_map_size -
                      h->m_meta_data_size) >>
                     LOG2_BLOCK_SIZE          ) - 1;

  num_used_blocks = cur_num_blocks -
                    h->m_free_count;

  if (num_blocks==0)                     // Automatic mode:
  {                                      // decide whether to
    if (!WORTH_SHRINK(num_used_blocks,   // shrink or not
                      cur_num_blocks))   // and, if yes,
      return true;                       // the new size

    num_blocks = EXTRA_GROWTH (num_used_blocks);
  }

  if (num_blocks > cur_num_blocks)
    return true;

  if (num_blocks>MAX_BLOCKS)
    num_blocks = MAX_BLOCKS;

  shf_assert (num_blocks >= num_used_blocks);

  data_size = num_blocks << LOG2_BLOCK_SIZE;

  expanded_size (data_size,         // Compute new layout of
                 map_size,          // data and metadata
                 meta_data_size,
                 true);

  // Compute range of blocks that will disappear

  start_del = (map_size >> LOG2_BLOCK_SIZE) - 1;

  if (start_del < (h->m_meta_data_size>>LOG2_BLOCK_SIZE))
    start_del = h->m_meta_data_size >> LOG2_BLOCK_SIZE;

  end_del = (h->m_map_size >> LOG2_BLOCK_SIZE) - 1;

  // Count _used_ nodes that need to be moved

  for (i=start_del, num_moving=0; i<end_del; i++)
    if (!PNODE(i)->is_free())
      num_moving ++;

  // The free nodes in that range can't be used.
  // They are "ex" free nodes

  num_free = end_del - start_del - num_moving;
  num_free = h->m_free_count - num_free;

  if (num_moving <= num_free)  // Enough really-free nodes?
  {
    uint32_t start_new, end_new;

    for (i=start_del; i<end_del; i++)  // First, take
      if (PNODE(i)->is_free())         // "ex" free nodes
        unfree_node (i);               // out of the list

    for (i=start_del; i<end_del; i++)  // Then use the
      if (!PNODE(i)->is_free())        // really free ones
        move_node (i, alloc_node());

    // Now, compute the number of free nodes that appear
    // as a result of reducing the metadata area

    start_new = meta_data_size >> LOG2_BLOCK_SIZE;
    end_new = h->m_meta_data_size >> LOG2_BLOCK_SIZE;

    if (end_new > ((map_size>>LOG2_BLOCK_SIZE)-1))
      end_new = (map_size>>LOG2_BLOCK_SIZE) - 1;

    // Save size changes in the header

    h->m_map_size = map_size;
    h->m_meta_data_size = meta_data_size;

    // Finally, add the afforementioned new free nodes.
    // Note that these couldn't be used before, since their
    // data blocks overlap with the final nodes of the
    // old metadata area.

    if (end_new > start_new)
      free_nodes_contiguous (start_new,
                             end_new - start_new);
  }
  else   // Not enough free nodes... defrag
  {
#ifdef NEVER_DEFRAG_ON_SHRINK
    return true;
#else

    uint32_t size[2], pos[2], tail, first, num;

    defrag (false);  // This will free a lot of blocks

    data_size = PDUMMY->m_bytes_subtree;
    defragmented_layout (data_size, meta_data_size,
                         size, pos);

    if (h->m_meta_data_size !=  // If the metadata area
        meta_data_size)         // changes, data must be moved
    {
      if (h->m_meta_data_size < data_size)   // Easiest case:
      {                                      // from 2 parts
        memcpy (m_pmap + meta_data_size,     // to 2 parts
                m_pmap + meta_data_size +
                         ROUND_TO_POW2_MULT(data_size,
                                            LOG2_BLOCK_SIZE),
                h->m_meta_data_size - meta_data_size);
      }
      else if (meta_data_size >= data_size)  // 1 -> 1
      {
        if (h->m_meta_data_size < meta_data_size + data_size)
          memmove (m_pmap + meta_data_size,
                   m_pmap + h->m_meta_data_size, // Maybe
                   data_size);                   // overlapped
        else                                     //    -> Move
          memcpy (m_pmap + meta_data_size,
                  m_pmap + h->m_meta_data_size,  // Otherwise
                  data_size);                    //    -> Copy
      }
      else                           // 1 part -> 2 parts
      {
        memcpy (m_pmap + meta_data_size,
                m_pmap + h->m_meta_data_size + size[0],
                size[1]);
                                             // Copy part 1
        if (pos[0] != h->m_meta_data_size)   // [move part 0]
          memmove (m_pmap + pos[0],
                   m_pmap + h->m_meta_data_size,
                   size[0]);
      }
    }

    // Save size changes in the header

    h->m_map_size = map_size;
    h->m_meta_data_size = meta_data_size;

    // Make new free nodes list and new tree

    h->m_free_list_first =
    h->m_free_list_last =
    h->m_free_count = 0;

    tail = ROUND_TO_POW2_MULT (meta_data_size+data_size,
                               LOG2_BLOCK_SIZE);

    free_nodes_contiguous
           ( tail >> LOG2_BLOCK_SIZE,
             ((map_size - tail) >> LOG2_BLOCK_SIZE) - 1 );

    first = pos[0] >> LOG2_BLOCK_SIZE;
    num = make_list_of_nodes (size, pos);
    build_tree (first, num);

#endif // NEVER_DEFRAG_ON_SHRINK
  }

  find_pos (m_abs_pos, m_cur_node, m_rel_pos);
  map_size = h->m_map_size;

  m_pfile->unmap ();
  m_pmap = NULL;

  if (!m_pfile->resize(map_size))   // TODO: error control
  {
    close (false);
    return false;
  }

  m_pmap = m_pfile->map ();

  if (!m_pmap)   // TODO: error control
  {
    close (false);
    return false;
  }

  check_integrity (true);
  return true;
}


/*  -------------------------------------------------------
    expanded_size() (static)

    Compute the map size required for a given amount of data.
    It computes the size of the metadata area.
    If min==false, the map size is rounded up to the total
    size that can be managed with the computed metadata size.
*/

void shiftable_file::expanded_size (uint32_t data_size,
                                    uint32_t & map_size,
                                    uint32_t & meta_data_size,
                                    bool min)
{
  uint32_t data_blocks, unusable_nodes;

  if (data_size==0)
    data_size = 1;

  unusable_nodes = DUMMY + 1;
  data_size = ROUND_TO_POW2_MULT (data_size, LOG2_BLOCK_SIZE);
  data_blocks = data_size >> LOG2_BLOCK_SIZE;

  for (;;)
  {
    meta_data_size = (unusable_nodes +
                      data_blocks      ) << LOG2_NODE_SIZE;

    meta_data_size = ROUND_TO_POW2_MULT (meta_data_size,
                                         LOG2_BLOCK_SIZE);

    if (unusable_nodes >= (meta_data_size >> LOG2_BLOCK_SIZE))
      break;

    unusable_nodes = meta_data_size >> LOG2_BLOCK_SIZE;
  }

  meta_data_size = unusable_nodes << LOG2_BLOCK_SIZE;

  map_size = min ?
      (meta_data_size + data_size) :
      (meta_data_size << (LOG2_BLOCK_SIZE - LOG2_NODE_SIZE));

  map_size += BLOCK_SIZE;  // Block for swap operations
}


/*  -------------------------------------------------------
    defragmented_layout() (static)

    Compute layout of a file in the less fragmented state
    possible. That is, the data of the beginning (which
    woud occupy the space used by the metadata) appended
    at the end (block aligned!). A second part stays in the
    same position it would occupy in a plain old file.
    Obvioulsy, this second part exists only if the data
    size is gtreater than the metadata size.
*/

void shiftable_file::defragmented_layout
                                  (uint32_t data_size,
                                   uint32_t meta_data_size,
                                   uint32_t size[2],
                                   uint32_t pos[2])
{
  if (data_size>meta_data_size)
  {                                   // Two parts
    size[0] = meta_data_size;           // 1st) beginning
    size[1] = data_size - size[0];      // 2nd) the rest
    pos[1] = size[0];
    pos[0] = ROUND_TO_POW2_MULT (pos[1]+size[1],
                                 LOG2_BLOCK_SIZE);
  }
  else
  {                                   // Just one part
    pos[0] = meta_data_size;
    size[0] = data_size;
    size[1] = 0;
  }
}


/*  -------------------------------------------------------
    disable_shrink()

    Switch off (or on) the feature of atomatically shrinking
    the file whenever the used space decreases below a specific
    proportion of the total occupied space (used+free). The
    parameter 'disable' idicates whether this feature has to
    be turned off (disable=true) or on (disable=false).
    Return old 'disable' value.
    Note that turning 'shrink' on (disable=false) might cause
    an inmediate shrink operation.
*/

bool shiftable_file::disable_shrink (bool disable/*=true*/)
{
  if (m_disable_shrink == disable)
    return disable;                        // No change

  m_disable_shrink = disable;              // Change

  if (disable)                             // If turning off,
    return false;                          // it's done

  PHEADER->set_current_op (OP_DELAYED_SHRINK);

  if (shrink())                            // If turning on,
    PHEADER->set_current_op (OP_NONE);     // try to shrink

  return true;
}


/*  -------------------------------------------------------
    stats()

    Return some file stats (information related to the space
    occupied, used/free blocks...) in the parameters passed
    by reference:

        map_size       - total bytes (metadata+used+free)
        meta_data_size - (no comments)
        used_count     - number of used blocks
        free_count     - number of free blocks
*/

void shiftable_file::stats (uint32_t & map_size,
                            uint32_t & meta_data_size,
                            uint32_t & used_count,
                            uint32_t & free_count) const
{
  header * h;

  if (!is_open())                 // If closed... all zero
  {
    map_size = meta_data_size =
    used_count = free_count = 0;

    return;
  }

  h = PHEADER;

  map_size = h->m_map_size;
  meta_data_size = h->m_meta_data_size;
  free_count = h->m_free_count;          // Compute used_count
                                         // from the other
  used_count = ( ( map_size -            // size-related data
                   meta_data_size ) >>
                 LOG2_BLOCK_SIZE       ) - 1 - free_count;
}

