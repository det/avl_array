///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal_posix.hpp
  --------------
  
  Declaration of a concrete OSAL (Operating System Abstraction
  Layer) that provides memory-mapped files in a Posix OS (like
  Linux).
*/

#ifndef _OSAL_POSIX_HPP_
#define _OSAL_POSIX_HPP_

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
    int m_fd;

    void init ()
    {
      m_pmap = NULL;
      m_mapped = m_open = false;
      m_size = 0;
      m_fd = -1;
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
