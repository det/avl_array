///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  osal.hpp
  --------
  
  Interface definition of an OSAL (Operating System Abstraction
  Layer) that must provide the basic services for the usage of
  memory-mapped files.
*/

#ifndef _OS_ABSTRACTION_LAYER_HPP_
#define _OS_ABSTRACTION_LAYER_HPP_

#include "../shiftable_files.hpp"

namespace shiftable_files
{

namespace detail
{

class file
{
  private:

    file (const file &)
    {}

    const file & operator= (const file &)
    { return *this; }

  public:

    file () {}
    virtual ~file () {}

    virtual bool open (const char * name, open_mode mode) = 0;
    virtual void close () = 0;
    virtual uint32_t size () = 0;
    virtual bool resize (uint32_t size) = 0;
    virtual int8_t * map () = 0;
    virtual void unmap () = 0;

    static file * new_file (file_type type);
};

} // namespace detail

} // namespace shiftable_files

#endif
