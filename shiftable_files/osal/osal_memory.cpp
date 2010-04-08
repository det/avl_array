///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_memory.cpp
  ---------------
  
  Implementation of the concrete OSAL declared in osal_memory.hpp
  (see comments there).
*/

#include "../detail/assert.hpp"
#include "osal_memory.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;

bool virtual_file::open (const char * name, open_mode mode)
{
  shf_assert (!m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pdata);

  if (name && *name)                       // Virtual files
    return false;                          // _must_ be
                                           // unnamed and
  if (mode != om_create_or_wipe_contents)  // new
    return false;

  m_open = true;
  m_size = 0;

  return true;
}

void virtual_file::close ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);

  free (m_pdata);
  init ();
}

uint32_t virtual_file::size ()
{
  return m_size;
}

bool virtual_file::resize (uint32_t size)
{
  int8_t * ptmp;

  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (size!=m_size);

  ptmp = (int8_t *) realloc (m_pdata, size?size:1);

  if (!ptmp)
    return false;

  m_size = size;
  m_pdata = ptmp;
  return true;
}

int8_t * virtual_file::map ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);

  if (!m_pdata)
    m_pdata = (int8_t *) malloc (1);

  if (!m_pdata)
    return NULL;

  m_mapped = true;
  return m_pdata;
}

void virtual_file::unmap ()
{
  shf_assert (m_open);
  shf_assert (m_mapped);
  shf_assert (m_pdata);

  m_mapped = false;
}

virtual_file::~virtual_file ()
{
  shf_assert (!m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pdata);

  if (m_mapped) unmap ();
  if (m_open) close ();
}
