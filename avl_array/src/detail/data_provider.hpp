///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2006-2009, Universidad de Alcala               //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  detail/data_provider.hpp
  ------------------------

  Functors used as parametric data sources. They return pointers to
  the objects that have to be copied. Depending on the context,
  NULL means "stop copying" or "use the default constructor".

  The user of the library doesn't need to know about these
  functors. They are for private use only.

  Available data providers are:

    class null_data_provider  (use the default constructor)
    iter_data_provider        (use a container)
    iter_aa_data_provider     (use an avl_array of the same type)
    range_data_provider       (same as iter_, but stop at "to")
    copy_data_provider        (use allways the same prototype)
*/

#ifndef _AVL_ARRAY_DATA_PROVIDER_HPP_
#define _AVL_ARRAY_DATA_PROVIDER_HPP_

#ifndef _AVL_ARRAY_HPP_
#error "Don't include this file. Include avl_array.hpp instead."
#endif

namespace mkr  // Public namespace
{

  namespace detail  // Private nested namespace mkr::detail
  {

//////////////////////////////////////////////////////////////////

template<class Ptr>          // Function object used for creating
class null_data_provider     // collections of default constructed
{                            // objects
  public:

    Ptr operator() () { return NULL; }  // Always NULL

    bool has_npsv () { return false; }
    template<class W> void get_npsv_width (W &) {}
};

//////////////////////////////////////////////////////////////////

template<class Ptr, class IT> // Function object used for copying
class iter_data_provider      // a container (or part of it)
{
#ifdef BOOST_CLASS_REQUIRE
  BOOST_CLASS_REQUIRE (IT, boost, InputIteratorConcept);
#endif

  private:

    IT it;                   // State: current position

  public:

    iter_data_provider (const IT & from) : it(from) {}

    Ptr operator() ()
    {
      Ptr p=&*it;      // Return current element and advance
      ++ it;
      return p;
    }

    bool has_npsv () { return false; }
    template<class W> void get_npsv_width (W &) {}
};

//////////////////////////////////////////////////////////////////

template<class Ptr, class IT, bool bW,
                              class W> // Function object used for
class iter_aa_data_provider            // copying an avl_array (or
{                                      // part of it)
#ifdef BOOST_CLASS_REQUIRE
  BOOST_CLASS_REQUIRE (IT, boost, InputIteratorConcept);
#endif

  private:

    IT it;                   // State: current position
    const W * last_width;    // NPSV width of last position

  public:

    iter_aa_data_provider (const IT & from) : it(from),
                                              last_width(NULL) {}

    Ptr operator() ()
    {
      Ptr p=&*it;                      // Return current element,
      last_width = & it.npsv_width (); // store a pointer to its
      ++ it;                           // NPSV width, and
      return p;                        // advance
    }

    bool has_npsv () { return bW; }
    void get_npsv_width (W & w) { w = *last_width; }
};

//////////////////////////////////////////////////////////////////

template<class Ptr, class IT> // Function object used for copying
class range_data_provider     // a container (or part of it)
{
#ifdef BOOST_CLASS_REQUIRE
  BOOST_CLASS_REQUIRE (IT, boost, InputIteratorConcept);
#endif

  private:

    IT it;                   // State: current position
    IT end;                  // Limit (to)

  public:

    range_data_provider (const IT & from,
                         const IT & to) : it(from), end(to) {}

    Ptr operator() ()
    {
      if (it==end)
        return NULL;

      Ptr p=&*it;      // Return current element and advance
      ++ it;
      return p;
    }

    bool has_npsv () { return false; }
    template<class W> void get_npsv_width (W &) {}
};

//////////////////////////////////////////////////////////////////

template<class Ptr>          // Function object used for creating
class copy_data_provider     // collections of copies of a given
{                            // element
  private:

    Ptr p;             // Original element

  public:

    copy_data_provider (Ptr x) : p(x) {}

    Ptr operator() () { return p; }    // Always the same

    bool has_npsv () { return false; }
    template<class W> void get_npsv_width (W &) {}
};

//////////////////////////////////////////////////////////////////

  }  // namespace detail

}  // namespace mkr

#endif

