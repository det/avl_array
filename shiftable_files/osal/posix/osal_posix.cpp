///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_posix.cpp
  --------------
  
  Implementation of a concrete OSAL (Operating System Abstraction
  Layer) that provides memory-mapped files in a Posix OS (like
  Linux).
*/

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../../detail/assert.hpp"
#include "osal_posix.hpp"
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
  off_t pos;

  shf_assert (!m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pmap);
  shf_assert (m_fd==-1);

  shf_assert (mode==om_create_or_wipe_contents ||
              mode==om_open_existing_or_fail);

  if (!name || !*name)
    return false;

  m_fd = ::open (name,
                 O_RDWR | (mode==om_create_or_wipe_contents ?
                           O_CREAT | O_TRUNC : 0),
                 S_IRUSR | S_IWUSR);

  if (m_fd==-1)
    return false;

  pos = lseek (m_fd, 0, SEEK_END);

  if (pos<0)
  {
    ::close (m_fd);
    m_fd = -1;
    return false;
  }

  m_size = (uint32_t) pos;
  m_open = true;
  return true;
}

void true_file::close ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (m_fd!=-1);

  ::close (m_fd);
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
  shf_assert (m_fd!=-1);

  if ((off_t)size < 0 ||
      ftruncate (m_fd, (off_t)size))
    return false;

  m_size = size;
  return true;
}

int8_t * true_file::map ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pmap);
  shf_assert (m_fd!=-1);

  m_pmap = (int8_t*) mmap (NULL,        // Any address is ok
                         (size_t)m_size,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,  // No copy-on-write
                         m_fd,
                         (off_t)0);   // No offset

  if (m_pmap==MAP_FAILED || !m_pmap)
  {
    m_pmap = NULL;
    return NULL;
  }

  m_mapped = true;
  return m_pmap;
}

void true_file::unmap ()
{
  shf_assert (m_open);
  shf_assert (m_mapped);
  shf_assert (m_pmap);

  munmap (m_pmap, (size_t)m_size);

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
