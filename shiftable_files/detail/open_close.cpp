///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  open_close.cpp
  --------------
  
  Implementation of open/close primitives, plus defragmentation
  routines.
  
  // TODO: Error control and recovery of damaged files in open()
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"
#include "../osal/osal.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;

void shiftable_file::init ()
{
  m_pmap = NULL;
  m_pfile = NULL;
  m_abs_pos = m_cur_node = m_rel_pos = 0;
  m_disable_shrink = false;
}

bool shiftable_file::open (const char * name/*=NULL*/,
                           open_mode mode/*=om_create...*/,
                           file_format format/*=ff_auto...*/)
{
  uint32_t file_size, data_size, meta_data_size, map_size,
           tail, size[2], pos[2], first, num;
  file_type type;
  header * h;

  if (is_open())
    return false;  // TODO: error control

  shf_assert (!m_pmap);

  type = name && *name ?
         ft_true_file : ft_virtual_file;

  m_pfile = file::new_file (type);

  if (!m_pfile)
    return false;  // TODO: error control

  if (!m_pfile->open (name, mode))  // TODO: error control
  {
    delete m_pfile;
    m_pfile = NULL;
    return false;
  }

  if (mode==om_create_or_wipe_contents)
  {
    format = ff_plain_file;
    file_size = 0;
  }
  else
    file_size = m_pfile->size ();

  if (format==ff_auto_detect)
  {
    if (file_size<sizeof(header))
      format = ff_plain_file;
    else
    {
      m_pmap = m_pfile->map ();

      if (!m_pmap)   // TODO: error control
      {
        m_pfile->close ();
        delete m_pfile;
        m_pfile = NULL;
        return false;
      }

      h = PHEADER;

      if (h->check_magic_bytes())
        format = ff_shiftable_file;
      else
      {
        m_pfile->unmap ();
        m_pmap = NULL;
        format = ff_plain_file;
      }
    }
  }

  if (format == ff_plain_file)
  {
    data_size = file_size;
    expanded_size (data_size, map_size, meta_data_size, true);

    if (!m_pfile->resize(map_size))   // TODO: error control
    {
      m_pfile->close ();
      delete m_pfile;
      m_pfile = NULL;
      return false;
    }

    m_pmap = m_pfile->map ();

    if (!m_pmap)   // TODO: error control
    {
      m_pfile->close ();
      delete m_pfile;
      m_pfile = NULL;
      return false;
    }

    // Move the beginning of the file to the end, leaving
    // space enough for header and nodes (meta data)

    if (data_size)
    {
      defragmented_layout (data_size, meta_data_size,
                           size, pos);

      memcpy (m_pmap+pos[0], m_pmap, size[0]);
    }
    else
      size[0] = pos[0] =
      size[1] = pos[1] = 0;

    // Clear meta data zone (header and nodes)

    memset (m_pmap, 0, meta_data_size);

    // Initialize header

    h = PHEADER;
    h->init ();

    h->m_map_size = map_size;
    h->m_meta_data_size = meta_data_size;

    // Initialize nodes

    tail = ROUND_TO_POW2_MULT (meta_data_size+data_size,
                               LOG2_BLOCK_SIZE);

    free_nodes_contiguous
             ( tail >> LOG2_BLOCK_SIZE,
               ((map_size - tail) >> LOG2_BLOCK_SIZE) - 1 );

    // Link current data with corresponding nodes

    first = pos[0] >> LOG2_BLOCK_SIZE;
    num = make_list_of_nodes (size, pos);
    build_tree (first, num);
  }
  else // format == ff_shiftable_file
  {
    if (!m_pmap)
    {
      m_pmap = m_pfile->map ();

      if (!m_pmap)   // TODO: error control
      {
        m_pfile->close ();
        delete m_pfile;
        m_pfile = NULL;
        return false;
      }
    }

    h = PHEADER;

    if (!h->check_magic_bytes() ||
        !h->verify_compatibility())    // TODO: error control
    {
      m_pfile->unmap ();
      m_pmap = NULL;
      m_pfile->close ();
      delete m_pfile;
      m_pfile = NULL;
      return false;
    }
  }

  if (h->m_state_flags != CLOSED_OK)  // TODO: error control
  {
    // TODO: !CLOSED_OK --> Try to fix it

    m_pfile->unmap ();
    m_pmap = NULL;
    m_pfile->close ();
    delete m_pfile;
    m_pfile = NULL;
    return false;
  }

  h->m_state_flags = OPEN;

  // Set current position (beginning)

  find_pos (m_abs_pos=0, m_cur_node, m_rel_pos);
  check_integrity (true);

  return true;
}

void shiftable_file::close (bool restore/*=true*/)
{
  uint32_t data_size=0, meta_data_size, map_size;

  if (!is_open())
    return;

  check_integrity (true);

  if (restore)
  {
    meta_data_size = PHEADER->m_meta_data_size;
    map_size = PHEADER->m_map_size;
    data_size = PDUMMY->m_bytes_subtree;

    if (data_size)
    {                  // Compact data and then overwrite meta
      defrag (false);  // data area with beginning of data

      if (data_size<=meta_data_size)
        memcpy (m_pmap, m_pmap+meta_data_size, data_size);
      else
        memcpy (m_pmap,
                m_pmap + ROUND_TO_POW2_MULT (data_size,
                                             LOG2_BLOCK_SIZE),
                meta_data_size);
    }

    m_pfile->unmap ();
    m_pmap = NULL;
    m_pfile->resize (data_size);  // TODO: error control
  }
  else
  {
    PHEADER->m_state_flags = CLOSED_OK;
    m_pfile->unmap ();
    m_pmap = NULL;
  }

  m_pfile->close ();  // TODO: error control
  delete m_pfile;
  init ();
}


/*  -------------------------------------------------------
    compact_data()

    // TODO: comment compact_data()
*/

void shiftable_file::compact_data (bool fix_tree)
{
  uint32_t data_size, meta_data_size, map_size,
           occup_blocks, cur, next, bytes;
  header * h;

  h = PHEADER;
  meta_data_size = h->m_meta_data_size;
  map_size = h->m_map_size;
  data_size = PDUMMY->m_bytes_subtree;

  occup_blocks = ( (map_size -
                    meta_data_size) >>
                   LOG2_BLOCK_SIZE     ) - 1
                                         - h->m_free_count;

  if ( ((occup_blocks-1) << LOG2_BLOCK_SIZE) +
       PNODE(PREV(DUMMY))->m_bytes         == data_size )
    return;

  cur = NEXT(DUMMY);
  next = NEXT(cur);

  while (next != DUMMY)
  {
    shf_assert (cur>DUMMY);
    shf_assert (next>DUMMY);

    if (PNODE(cur)->m_bytes < BLOCK_SIZE)
    {
      do
      {
        bytes = BLOCK_SIZE - PNODE(cur)->m_bytes;

        if (bytes > PNODE(next)->m_bytes)
          bytes = PNODE(next)->m_bytes;

        memcpy (PBLOCK(cur)+PNODE(cur)->m_bytes,
                PBLOCK(next),
                bytes);

        PNODE(cur)->m_bytes += bytes;

        if (bytes == PNODE(next)->m_bytes)
        {
          NEXT(PREV(next)) = NEXT(next);
          PREV(NEXT(next)) = PREV(next);
          free_node (next);
          next = NEXT(cur);
          occup_blocks --;

          if (next == DUMMY)
            break;
        }
      }
      while (PNODE(cur)->m_bytes < BLOCK_SIZE);

      if (next == DUMMY)
        break;

      memmove (PBLOCK(next),
               PBLOCK(next)+bytes,
               PNODE(next)->m_bytes-bytes);

      PNODE(next)->m_bytes -= bytes;
    }

    shf_assert (next>DUMMY);

    cur = next;
    next = NEXT(cur);
  }

  shf_assert ( ((occup_blocks-1) << LOG2_BLOCK_SIZE) +
               PNODE(PREV(DUMMY))->m_bytes
                                            == data_size );
  if (fix_tree)
  {
    build_tree (NEXT(DUMMY), occup_blocks);
    check_integrity ();
    find_pos (m_abs_pos, m_cur_node, m_rel_pos);
  }
}


/*  -------------------------------------------------------
    defrag() // TODO: Comment it!

    Sort blocks and compact data. Except for very short
    files, break the sequence in two parts and place the
    second part first and the first part in the end. That
    first part must have the size of the metadata. This
    way, a simple memcpy() after defrag(), can convert
    the shiftable_file to a plain old file.

    If fix_tree==false, don't waste time with the metadata,
    cause it will be overwritten in short.
*/

void shiftable_file::defrag (bool fix_tree)
{
  uint32_t data_size, meta_data_size, map_size,
           size[2], pos[2], part, part_pos, part_size,
           first, num, prev, next,
           source_node, source_pos, dest_node, dest_pos,
           room, bytes, copying;
  header * h;

  h = PHEADER;
  meta_data_size = h->m_meta_data_size;
  map_size = h->m_map_size;
  data_size = PDUMMY->m_bytes_subtree;

  if (!data_size)
    return;

  defragmented_layout (data_size, meta_data_size,
                       size, pos);
  part = 0;
  part_pos = pos[part];
  part_size = size[part];

  source_pos = 0;
  source_node = PDUMMY->m_next;

  do
  {
    shf_assert (part_size>0);

    dest_pos = 0;
    dest_node = part_pos >> LOG2_BLOCK_SIZE;

    if (dest_node != source_node)
    {
      if (PNODE(dest_node)->is_free())
        unfree_node (dest_node);
      else if (h->m_free_count)
      {
        num = alloc_node ();
        move_node (dest_node, num, true, false);
      }
      else
      {
        swap_nodes (dest_node, source_node);
        source_node = dest_node;
      }
    }

    if (dest_node == source_node)
    {
      if (source_pos)
      {
        memmove (PBLOCK(source_node),
                 PBLOCK(source_node)+source_pos,
                 PNODE(source_node)->m_bytes-source_pos);

        PNODE(source_node)->m_bytes -= source_pos;
      }

      dest_pos = PNODE(dest_node)->m_bytes;

      source_node = NEXT(source_node);
      source_pos = 0;
    }

    while (dest_pos < BLOCK_SIZE &&
           source_node != DUMMY)
    {
      shf_assert (dest_node!=source_node);
      shf_assert (PNODE(source_node)->m_bytes);
      shf_assert (source_node>DUMMY);
      shf_assert (PBLOCK(source_node)<PSWAP);
      shf_assert (dest_pos<BLOCK_SIZE);
      shf_assert (source_pos<BLOCK_SIZE);

      room = BLOCK_SIZE - dest_pos;
                                            // Compute amount
      bytes = PNODE(source_node)->m_bytes - // of contiguous
              source_pos;                   // bytes that can
                                            // be copied
      copying = room<bytes ? room : bytes;

      shf_assert (copying>0);
      shf_assert (copying<=BLOCK_SIZE);

      memcpy (PBLOCK(dest_node)+dest_pos,
              PBLOCK(source_node)+source_pos,
              copying);

      dest_pos += copying;         // Advance
      source_pos += copying;

      if (source_pos==PNODE(source_node)->m_bytes)
      {
        next = NEXT(source_node);
        prev = PREV(source_node);
        NEXT(prev) = next;
        PREV(next) = prev;
        free_node (source_node);
        source_node = next;
        source_pos = 0;
      }
    }

    part_pos += dest_pos;
    part_size -= dest_pos;          // Advance

    if (part==0 && part_size==0 && size[1]>0)  // Next part?
    {
      part ++;
      part_pos = pos[part];
      part_size = size[part];
    }
  }
  while (part_size>0);

  shf_assert (source_node==DUMMY);

  if (!fix_tree)
    return;

  first = pos[0] >> LOG2_BLOCK_SIZE;
  num = make_list_of_nodes (size, pos);    // Fix tree
  build_tree (first, num);

  check_integrity ();
                                               // Fix cur.
  find_pos (m_abs_pos, m_cur_node, m_rel_pos); // position
}


/*  -------------------------------------------------------
    move_node()

    Move a node of the tree from a position to another (both
    specified as node numbers). Move accordingly its payload
    data block.
*/

void shiftable_file::move_node (uint32_t from,
                                uint32_t to,
                                bool block_too/*=true*/,
                                bool fix_tree/*=true*/)
{
  node * n;

  if (from==to)
    return;

  shf_assert (from>DUMMY);
  shf_assert (to>DUMMY);

  memcpy (n=PNODE(to), PNODE(from), NODE_SIZE);

  shf_assert (!fix_tree || n->m_parent);
  shf_assert (n->m_next);
  shf_assert (n->m_prev);
  shf_assert (n->m_bytes<=BLOCK_SIZE);

  if (block_too)
    memcpy (PBLOCK(to), PBLOCK(from), n->m_bytes);

  if (fix_tree)
  {
    if (n->child(L)!=0)
      PARENT(n->child(L)) = to;

    if (n->child(R)!=0)
      PARENT(n->child(R)) = to;

    if (LEFT(n->m_parent)==from)
      LEFT(n->m_parent) = to;
    else
      RIGHT(n->m_parent) = to;
  }

  PREV(n->m_next) = to;
  NEXT(n->m_prev) = to;
}


/*  -------------------------------------------------------
    swap_nodes()

    Helper function for defrag().
    Swap two nodes of the tree (both specified as node
    numbers) and their payload data blocks. Don't waste
    time with the parent/child links. Just keep updated
    the next/prev links.
*/

void shiftable_file::swap_nodes (uint32_t a,
                                 uint32_t b)
{
  uint32_t c;
  int8_t * swap;
  node tmp;

  if (a==b)
    return;

  shf_assert (a>DUMMY);
  shf_assert (b>DUMMY);

  if (PNODE(a)->m_bytes > PNODE(b)->m_bytes)
  {
    c = a;
    a = b;
    b = c;
  }

  swap = PSWAP;

  memcpy (swap,      PBLOCK(a), PNODE(a)->m_bytes);
  memcpy (PBLOCK(a), PBLOCK(b), PNODE(b)->m_bytes);
  memcpy (PBLOCK(b), swap,      PNODE(a)->m_bytes);

  memcpy (&tmp,     PNODE(a), NODE_SIZE);
  memcpy (PNODE(a), PNODE(b), NODE_SIZE);
  memcpy (PNODE(b), &tmp,     NODE_SIZE);

  if (NEXT(a) == a)        // Was b-a, will be a-b
  {
    shf_assert (NEXT(b) != b);

    NEXT(a) = b;
    PREV(b) = a;

    NEXT(PREV(a)) = a;
    PREV(NEXT(b)) = b;
  }
  else if (NEXT(b) == b)   // Was a-b, will be b-a
  {
    NEXT(b) = a;
    PREV(a) = b;

    PREV(NEXT(a)) = a;
    NEXT(PREV(b)) = b;
  }
  else      // Was a-...-b or b-...-a
  {
    PREV(NEXT(a)) = a;
    NEXT(PREV(a)) = a;

    PREV(NEXT(b)) = b;
    NEXT(PREV(b)) = b;
  }
}


/*  -------------------------------------------------------
    make_list_of_nodes()

    Helper function for open() and defrag().
    Make a linked list with the nodes of one or two segments
    of data. Tipically, the first segment is the beginning
    of the payload data, which is moved to the end in order
    to make room for the metadata. The second segment is the
    part that stays in its place.
*/

uint32_t shiftable_file::make_list_of_nodes (uint32_t size[2],
                                             uint32_t pos[2])
{
  uint32_t part, cur, to, last, num, last_bytes;

  if (size[0]==0)
    return 0;

  shf_assert (size[1]==0 ||
              (size[0]&(BLOCK_SIZE-1))==0);

  last_bytes = (size[0]+size[1]) & (BLOCK_SIZE-1);

  if (last_bytes==0)
    last_bytes = BLOCK_SIZE;

  // Translate size and pos to nodes numbers

  size[0] = (size[0]+BLOCK_SIZE-1) >> LOG2_BLOCK_SIZE;
  size[1] = (size[1]+BLOCK_SIZE-1) >> LOG2_BLOCK_SIZE;

  pos[0] >>= LOG2_BLOCK_SIZE;
  pos[1] >>= LOG2_BLOCK_SIZE;

  num = size[0] + size[1];

  part = 0;
  cur = pos[part];
  to = pos[part] + size[part];

  for (;;)
  {
    last = cur ++;

    if (cur==to)
    {
      if (part==1 || size[1]==0)
      {
        NEXT(last) = 0;
        PNODE(last)->m_bytes = last_bytes;
        return num;
      }

      part = 1;
      cur = pos[part];
      to = pos[part] + size[part];
    }

    NEXT(last) = cur;
    PNODE(last)->m_bytes = BLOCK_SIZE;
  }
}

