///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_windows.hpp
  ----------------
  
  Declaration of a concrete OSAL (Operating System Abstraction
  Layer) that provides memory-mapped files in Windows.
*/

#ifndef _OSAL_WINDOWS_HPP_
#define _OSAL_WINDOWS_HPP_

#include <windows.h>  // Windows SDK required
#include "../osal.hpp"

namespace shiftable_files
{

namespace detail
{

class true_file : public file
{
  private:

    bool m_open;
    int8_t * m_pmap;
    bool m_mapped;
    uint32_t m_size;
    HANDLE m_hfile;
    HANDLE m_hmap;

    void init ()
    {
      m_pmap = NULL;
      m_mapped = m_open = false;
      m_size = 0;
      m_hfile = m_hmap = INVALID_HANDLE_VALUE;
    }

    true_file (const true_file &)
    { init (); }

    const true_file & operator= (const true_file &)
    { return *this; }

  public:

    true_file () { init (); }
    ~true_file ();

    bool open (const char * name, open_mode mode);
    void close ();
    uint32_t size ();
    bool resize (uint32_t size);
    int8_t * map ();
    void unmap ();
};

} // namespace detail

} // namespace shiftable_files

#endif
