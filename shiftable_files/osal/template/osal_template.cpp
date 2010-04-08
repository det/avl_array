///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_WHATEVER.cpp
  -----------------
  
  Implementation of a concrete OSAL (Operating System Abstraction
  Layer) that provides memory-mapped files in WHATEVER OS.
  
  // DO: Change file name, and specify the OS' name above.
*/

#error "This is just a template. It should never be compiled."
// DO: remove the directive above

#include "../../detail/assert.hpp"
#include "osal_template.hpp"    // DO: Change header name
#include "../osal_memory.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;

file * file::new_file (file_type type)
{
  switch (type)
  {
    case ft_true_file: return new true_file;
    case ft_virtual_file: return new virtual_file;
    default: return NULL;
  }
}

bool true_file::open (const char * name, open_mode mode)
{
  shf_assert (!m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pmap);

  shf_assert (mode==om_create_or_wipe_contents ||
              mode==om_open_existing_or_fail);

  if (!name || !*name)
    return false;

  // DO: Open the file (store file descriptor)
  //     If error, just return false
  //     Otherwise...

  // DO: Get file size and store it in m_size

  m_open = true;
  return true;
}

void true_file::close ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);

  // DO: Close the file

  init ();
}

uint32_t true_file::size ()
{
  return m_size;
}

bool true_file::resize (uint32_t size)
{
  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (size!=m_size);

  // DO: Change the size of the file
  //     If error, return false

  m_size = size;
  return true;
}

int8_t * true_file::map ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pmap);

  // DO: Map the file
  //     If error, return NULL
  //     Otherwise, store map address in m_pmap

  m_mapped = true;
  return m_pmap;
}

void true_file::unmap ()
{
  shf_assert (m_open);
  shf_assert (m_mapped);
  shf_assert (m_pmap);

  // DO: Unmap the file

  m_pmap = NULL;
  m_mapped = false;
}

true_file::~true_file ()
{
  shf_assert (!m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pmap);

  if (m_mapped) unmap ();
  if (m_open) close ();
}
