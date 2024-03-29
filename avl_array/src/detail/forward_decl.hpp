///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2006-2009, Universidad de Alcala               //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  detail/forward_decl.hpp
  -----------------------

  Forward declarations
*/

#ifndef _AVL_ARRAY_FORWARD_DECL_HPP_
#define _AVL_ARRAY_FORWARD_DECL_HPP_

#ifndef _AVL_ARRAY_HPP_
#error "Don't include this file. Include avl_array.hpp instead."
#endif

namespace mkr  // Public namespace
{

  template<class T, class A,
           bool bW, class W,
           bool bP, class P>         // The only visible class
  class avl_array;                   // is avl_array<T,A,bW,W,bP,P>

  namespace detail  // Private nested namespace mkr::detail
  {

    template<class T, class A,
             bool bW, class W,
             bool bP, class P>            // Links and counters
    class avl_array_node_tree_fields;     // of a tree node

    template<class T, class A,
             bool bW, class W,
             bool bP, class P>            // A tree node, including
    class avl_array_node;                 // its payload value_type

    template<class T, class A,
             bool bW, class W,
             bool bP, class P>            // A list of nodes to
    class rollback_list;                  // complete or delete

    template<class T, class A,
             bool bW, class W,
             bool bP, class P,
             class Ref, class Ptr>
    class avl_array_iterator;             // Normal iterator

    template<class T, class A,
             bool bW, class W,
             bool bP, class P,
             class Ref, class Ptr>
    class avl_array_rev_iter;             // Reverse iterator

    class allocator_returned_null;
    class index_out_of_bounds;
    class invalid_op_with_end;            // Exceptions
    class lesser_and_greater;

    template<class Ptr>
    class null_data_provider;
    template<class Ptr, class IT>           // Functors used
    class iter_data_provider;               // internally for
    template<class Ptr, class IT, bool bW,  // encapsulating
                                  class W>  // different sources of
    class iter_aa_data_provider;            // value_type objects
    template<class Ptr, class IT>           // under the same
    class range_data_provider;              // interface
    template<class Ptr>
    class copy_data_provider;

  }  // namespace detail

}  // namespace mkr

#endif

