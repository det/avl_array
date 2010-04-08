///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_any.cpp
  ------------
  
  Implementation of the concrete OSAL declared in osal_any.hpp
  (see comments there).
*/

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include "../../detail/assert.hpp"
#include "osal_any.hpp"
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
  FILE * pf;
  bool ok;

  shf_assert (!m_open);
  shf_assert (m_name=="");
  shf_assert (!m_mapped);
  shf_assert (!m_pdata);

  shf_assert (mode==om_create_or_wipe_contents ||
              mode==om_open_existing_or_fail);

  if (!name || !*name)
    return false;

  pf = fopen (name,
              mode==om_create_or_wipe_contents ?
              "wb+" : "rb+");
  if (!pf)
    return false;

  m_open = true;
  m_name = name;

  ok = fseek (pf, 0, SEEK_END) == 0;
  m_size = ftell (pf);

  ok = ok && (m_pdata = (int8_t*) malloc (m_size?m_size:1)) != NULL
          && ( m_size == 0 ||
               m_size == fread (m_pdata, 1, m_size, pf) );

  fclose (pf);

  if (ok)
    return true;

  free (m_pdata);
  init ();
  return false;
}

void true_file::close ()
{
  FILE * pf;

  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (m_pdata);

  pf = fopen (m_name.c_str(), "wb");

  if (pf)
  {
    if (m_size)
      fwrite (m_pdata, 1, m_size, pf);

    fclose (pf);
  }

  free (m_pdata);
  init ();
}

uint32_t true_file::size ()
{
  return m_size;
}

bool true_file::resize (uint32_t size)
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

int8_t * true_file::map ()
{
  shf_assert (m_open);
  shf_assert (!m_mapped);
  shf_assert (m_pdata);

  m_mapped = true;
  return m_pdata;
}

void true_file::unmap ()
{
  shf_assert (m_open);
  shf_assert (m_mapped);
  shf_assert (m_pdata);

  m_mapped = false;
}

true_file::~true_file ()
{
  shf_assert (!m_open);
  shf_assert (!m_mapped);
  shf_assert (!m_pdata);

  if (m_mapped) unmap ();
  if (m_open) close ();
}
