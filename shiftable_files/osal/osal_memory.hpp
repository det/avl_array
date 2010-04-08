///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_memory.hpp
  ---------------
  
  Routines that provide dynamic memory instead of memory-mapped
  files. These 'virtual' files will be used whenever the open()
  method of shiftable_file is called with an emty file name
  (either NULL or ""). The difference with respect to osal_any
  is that virtual files are always empty when opened, and their
  contents simply disappear when they are closed.
  
  IMPORTANT: Note that this 'false' memory-mapping OSAL alters
             completely the performance of shiftable_files.
*/

#ifndef _OSAL_MEMORY_HPP_
#define _OSAL_MEMORY_HPP_

#include <cstdlib>

#include "osal.hpp"

namespace shiftable_files
{

namespace detail
{

class virtual_file : public file
{
  private:

    bool m_open;
    int8_t * m_pdata;
    bool m_mapped;
    uint32_t m_size;

    void init ()
    {
      m_pdata = NULL;
      m_mapped = m_open = false;
      m_size = 0;
    }

    virtual_file (const virtual_file &)
    { init (); }

    const virtual_file & operator= (const virtual_file &)
    { return *this; }

  public:

    virtual_file () { init (); }
    ~virtual_file ();

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
