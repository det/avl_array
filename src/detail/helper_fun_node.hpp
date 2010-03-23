///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2006-2009, Universidad de Alcala               //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  detail/helper_fun_node.hpp
  --------------------------

  Private helper methods for the relationship container-nodes

  dummy(): get the sentinel/end node of this container (O(1))
  dummy_ownwer(): get the container of a dummy node (O(1))
  owner(): get the container of a node (O(log N))
*/

#ifndef _AVL_ARRAY_HELPER_FUN_NODE_HPP_
#define _AVL_ARRAY_HELPER_FUN_NODE_HPP_

#ifndef _AVL_ARRAY_HPP_
#error "Don't include this file. Include avl_array.hpp instead."
#endif

namespace mkr  // Public namespace
{

//////////////////////////////////////////////////////////////////

// Most data stored inside the avl_array object come from the base
// class: a node. This node is the root of the whole tree and the
// end element at the same time. It is the only node whose parent
// pointer is NULL. Its right child pointer will allways be NULL.
// Last, but not least, it has no T data. The method dummy()
// returns a pointer to this sentinel/end node
//
// Complexity: O(1)

template<class T,class A,bool bW,class W,bool bP,class P>
inline
  typename avl_array<T,A,bW,W,bP,P>::node_t *
  avl_array<T,A,bW,W,bP,P>::dummy () const
{
  return static_cast<node_t*> (
         const_cast<my_class*> (this) );
}

// The static method dummy_owner() calculates the
// address of an avl_array from its dummy's address
//
// Complexity: O(1)

template<class T,class A,bool bW,class W,bool bP,class P>
inline //static
  typename avl_array<T,A,bW,W,bP,P>::my_class *
  avl_array<T,A,bW,W,bP,P>::dummy_owner
  (const typename avl_array<T,A,bW,W,bP,P>::node_t * pdummy)
{
  AA_ASSERT (!pdummy->m_parent);

  return static_cast<my_class*> (
         const_cast<node_t*> (pdummy) );
}

// Finally, the static method owner() returns the address
// of the avl_array to which a node belongs. This takes
// some time
//
// Complexity: O(log N)

template<class T,class A,bool bW,class W,bool bP,class P>
inline //static
  typename avl_array<T,A,bW,W,bP,P>::my_class *
  avl_array<T,A,bW,W,bP,P>::owner
  (const typename avl_array<T,A,bW,W,bP,P>::node_t * node)
{
  while (node->m_parent)
    node = node->m_parent;

  return dummy_owner (node);
}

//////////////////////////////////////////////////////////////////

}  // namespace mkr

#endif

