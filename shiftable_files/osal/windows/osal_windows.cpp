///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_windows.cpp
  ----------------
  
  Implementation of a concrete OSAL (Operating System Abstraction
  Layer) that provides memory-mapped files in Windows.

      // NOTE: the file mapping object is created/destroyed
               every time. The exact maximum size is specified
               on creation. Though, this is probably flushing
               all pages on resize... Keeping the file mapping
               object all the time (creating/destroying only
               the map views), would be more efficient. The
               drawback is that, in this case, specifying a
               huge maximum size would consume too much
               virtual address space.
*/

#include "../../detail/assert.hpp"
#include "osal_windows.hpp"
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
  LARGE_INTEGER size;

  shf_assert (!m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pmap);

  shf_assert (mode==om_create_or_wipe_contents ||
              mode==om_open_existing_or_fail);

  if (!name || !*name)
    return false;

  m_hfile = CreateFileA
             (name,
              GENERIC_READ | GENERIC_WRITE,
              0,     // Exclusive access
              NULL,  // No security attr., no inherit
              mode==om_create_or_wipe_contents ?
                  CREATE_ALWAYS : OPEN_EXISTING,
              FILE_ATTRIBUTE_NORMAL |
                  FILE_FLAG_RANDOM_ACCESS,
              NULL); // No template

  if (m_hfile == INVALID_HANDLE_VALUE)
    return false;

  if (!GetFileSizeEx(m_hfile,&size) ||
      size.HighPart ||
      size.LowPart > MAX_SIZE)
  {
    CloseHandle (m_hfile);
    m_hfile = INVALID_HANDLE_VALUE;
    return false;
  }

  m_size = size.LowPart;
  m_open = true;
  return true;
}

void true_file::close ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);

  CloseHandle (m_hfile);
  init ();
}

uint32_t true_file::size ()
{
  return m_size;
}

bool true_file::resize (uint32_t size)
{
  LARGE_INTEGER large_size;

  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (size!=m_size);

  large_size.HighPart = 0;
  large_size.LowPart = size;

  if (!SetFilePointerEx (m_hfile, large_size,
                        NULL, FILE_BEGIN) ||
      !SetEndOfFile (m_hfile))
    return false;

  m_size = size;
  return true;
}

int8_t * true_file::map ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pmap);
  shf_assert (m_size);

  m_hmap = CreateFileMapping
             (m_hfile,
              NULL,            // No inherit
              PAGE_READWRITE,
              0,               // Size (high)
              m_size,          // Size (low)
              NULL);           // Name of mapping

  if (m_hmap == NULL)
    return NULL;

  m_pmap = (int8_t*) MapViewOfFile
             (m_hmap,
              FILE_MAP_ALL_ACCESS,
              0,                   // Offset (high)
              0,                   // Offset (low)
              m_size);

  if (m_pmap)
    m_mapped = true;
  else
  {
    CloseHandle (m_hmap);
    m_hmap = INVALID_HANDLE_VALUE;
  }

  return m_pmap;
}

void true_file::unmap ()
{
  shf_assert (m_open);
  shf_assert (m_mapped);
  shf_assert (m_pmap);

  UnmapViewOfFile (m_pmap);
  CloseHandle (m_hmap);
  m_hmap = INVALID_HANDLE_VALUE;

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
