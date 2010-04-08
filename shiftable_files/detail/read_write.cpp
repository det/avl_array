///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  read_write.cpp
  --------------
  
  Implementation of read/write primitives.
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;


/*  -------------------------------------------------------
    read()

    Read some bytes from the shiftable file, and write them
    in the specified memory buffer. Start reading at the
    current position, and advance (change current position)
    to the end of the bytes read.
*/

uint32_t shiftable_file::read (void * buff,
                               uint32_t bytes)
{
  if (!is_open() ||
      bytes==0 || m_cur_node==DUMMY)
    return 0;

  check_integrity (true);

  if (bytes < PNODE(m_cur_node)->m_bytes - m_rel_pos)
  {
    memcpy (buff, PBLOCK(m_cur_node)+m_rel_pos, bytes);
    m_rel_pos += bytes;
    m_abs_pos += bytes;  // If the bytes to read were
    return bytes;        // in a single block... it's done!
  }

  int8_t * pbuff;    // Current position in mem buffer
  uint32_t pending;  // Bytes that still have to be copied
  uint32_t size;     // Bytes to copy in a row
  node * pcur;       // Current node in shiftable file

  pbuff = (int8_t*) buff;
  pending = bytes;

  pcur = PNODE(m_cur_node);
  size = pcur->m_bytes - m_rel_pos;

  if (size)
    memcpy (pbuff, PBLOCK(m_cur_node)+ // Copy till end of
                   m_rel_pos, size);   // current block

  m_rel_pos = 0;               // New position: beginnig...

  for (;;)
  {
    m_cur_node = pcur->m_next; // ... of the next block
    pbuff += size;             // Advance in memory buffer
    pending -= size;           // Discount bytes copied

    if (m_cur_node==DUMMY ||
        pending==0)            // No more data to read...
      break;                   // ... done

    pcur = PNODE(m_cur_node);
    size = pcur->m_bytes - m_rel_pos;

    if (pending<size)  // If this block has enough bytes
    {
      memcpy (pbuff, PBLOCK(m_cur_node), pending);
      m_rel_pos = pending;
      pending = 0;             // Copy them and... done
      break;
    }

    memcpy (pbuff,               // Copy till end of
            PBLOCK(m_cur_node),  // current block
            size);
  }                            // Go to the next block

  bytes -= pending;
  m_abs_pos += bytes;
  return bytes;
}


/*  -------------------------------------------------------
    write()

    Take some bytes from the specified buffer, and write
    (overwrite or, if necessary, append) them in the
    shiftable file. Start writing at the current position,
    and advance (change current position) to the end of the
    bytes written.
*/

uint32_t shiftable_file::write (const void * buff,
                                uint32_t bytes)
{
  if (!is_open())
    return 0;

  if (m_abs_pos+bytes < m_abs_pos ||   // >4GB...
      m_abs_pos+bytes < bytes ||       // ...overflow
      m_abs_pos+bytes > MAX_SIZE)
    return 0;

  if (bytes==0)
  {
    if (m_abs_pos > PDUMMY->m_bytes_subtree)
      resize (m_abs_pos);

    return 0;
  }

  check_integrity (true);

  if (m_abs_pos+bytes > PDUMMY->m_bytes_subtree)
    if (!resize(m_abs_pos+bytes))
      return 0;

  // Now write (just like reading, but changing
  // the parameters of memcpy)

  if (bytes < PNODE(m_cur_node)->m_bytes - m_rel_pos)
  {
    memcpy (PBLOCK(m_cur_node)+m_rel_pos, buff, bytes);
    m_rel_pos += bytes;
    m_abs_pos += bytes;
    return bytes;
  }
                                  // - see comments in read() -
  const int8_t * pbuff;
  uint32_t pending, size;
  node * pcur;

  pbuff = (const int8_t*) buff;
  pending = bytes;

  pcur = PNODE(m_cur_node);
  size = pcur->m_bytes - m_rel_pos;

  if (size)
    memcpy (PBLOCK(m_cur_node)+m_rel_pos, pbuff, size);

  m_rel_pos = 0;

  for (;;)
  {                               // - see comments in read() -
    m_cur_node = pcur->m_next;
    pbuff += size;
    pending -= size;

    if (m_cur_node==DUMMY || pending==0)
      break;

    pcur = PNODE(m_cur_node);
    size = pcur->m_bytes - m_rel_pos;

    if (pending<size)
    {
      memcpy (PBLOCK(m_cur_node), pbuff, pending);
      m_rel_pos = pending;
      pending = 0;
      break;
    }

    memcpy (PBLOCK(m_cur_node), pbuff, size);
  }

  bytes -= pending;               // - see comments in read() -
  m_abs_pos += bytes;
  return bytes;
}
