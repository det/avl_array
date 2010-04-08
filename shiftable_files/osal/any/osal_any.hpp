///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_any.hpp
  ------------
  
  Declaration of a concrete OSAL (Operating System Abstraction
  Layer) that can 'simulate' memory-mapped files in any OS that
  does _not_ provide them.
  
  This OSAL does _not_ actually map files in virtual memory.
  Instead, it loads them in dynamic memory (completely) and
  writes them back to disk at the end. It allows using
  shiftable_files in platforms without support for memory-mapped
  files.
  
  IMPORTANT: Note that this 'false' memory-mapping OSAL alters
             completely the performance of shiftable_files.
*/

#ifndef _OSAL_ANY_HPP_
#define _OSAL_ANY_HPP_

#include <cstdio>
#include <string>

#include "../osal.hpp"

namespace shiftable_files
{

namespace detail
{

class true_file : public file
{
  private:

    bool m_open;
    std::string m_name;
    int8_t * m_pdata;
    bool m_mapped;
    uint32_t m_size;

    void init ()
    {
      m_pdata = NULL;
      m_name = "";
      m_mapped = m_open = false;
      m_size = 0;
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
