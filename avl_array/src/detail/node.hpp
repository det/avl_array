///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2006-2009, Universidad de Alcala               //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  detail/node.hpp
  ---------------

  The class avl_array_node_tree_fields, defined here, contains all
  the links required by tree nodes. It does _not_ contain the
  payload value_type (see detail/node_with_data.hpp).

  Two classes inherit from avl_array_node_tree_fields:

    avl_array_node   (see detail/node_with_data.hpp)
    avl_array        (see avl_array.hpp)
*/

#ifndef _AVL_ARRAY_NODE_HPP_
#define _AVL_ARRAY_NODE_HPP_

#ifndef _AVL_ARRAY_HPP_
#error "Don't include this file. Include avl_array.hpp instead."
#endif

#if !defined(__GNUC__) || defined(__MINGW32__)
#define AA_NO_ZERO_SIZE_ARRAYS
#endif

namespace mkr  // Public namespace
{

  namespace detail  // Private nested namespace mkr::detail
  {

//////////////////////////////////////////////////////////////////
/*
  The type enum_left_right declares L and R.
  Every tree node stores the pointers to its children in an array
  of two pointers. L and R (constant values) are often used as
  index in the array, but in some cases the index is a variable.
  We can take advantage of symmetry: if children[x] is one side,
  children[1-x] is the other one.
*/

typedef enum { L=0, R=1 } enum_left_right;

//////////////////////////////////////////////////////////////////

template<class T, class A,        // Data of a tree node (payload
         bool bW, class W,        // not included)
         bool bP, class P>
class avl_array_node_tree_fields
{                                 // Note that the dummy has no T

  friend class mkr::avl_array<T,A,bW,W,bP,P>;
  friend class rollback_list<T,A,bW,W,bP,P>;

  typedef avl_array_node_tree_fields<T,A,bW,W,bP,P>    node_t;

  protected:

    // Tree links (parent of root and children of leafs are NULL)

    node_t * m_parent;      // Parent node
    node_t * m_children[2]; // [0]:left [1]:right

    // Circular doubly linked list (equiv. to in-order travel)

    node_t * m_next;        // (last_node.next==dummy)
    node_t * m_prev;        // (first_node.prev==dummy)

    // Data for balancing, indexing, and stable-sort

    std::size_t m_height;   // Levels in subtree, including self
    std::size_t m_count;    // Nodes in subtree, including self

#ifndef AA_NO_ZERO_SIZE_ARRAYS
    P m_oldpos[bP?1:0];     // Position (used only in stable_sort)
#else
    P m_oldpos[1];          // (zero size arrays are not standard)
#endif

    // Non-Proportional Sequence View (NPSV)

#ifndef AA_NO_ZERO_SIZE_ARRAYS
    W m_node_width[bW?1:0];   // Width of this node
    W m_total_width[bW?1:0];  // Width of this subtree (if there
                              // are no children, m_node_width is
#else                         // used instead)
    W m_node_width[1];
    W m_total_width[1];     // (zero size arrays are not standard)
#endif

    // Constructor and initializer, both O(1)

    avl_array_node_tree_fields ();     // Default constructor
    void init_tree_fields ();          // Write default values
    void init ();                      // Init all (width too)

    // Helper functions, all O(1) (no loop, no recursion)

    std::size_t left_count () const;   // Nodes in left subtree
    std::size_t right_count () const;  // Nodes in right subtree

    std::size_t left_height () const;  // Height of left subtree
    std::size_t right_height () const; // Height of right subtree

    void get_left_width (W&) const;    // Width of left subtree
    void get_right_width (W&) const;   // Width of right subtree

    const W & total_width () const;    // Width of subtree

    void update_width ();              // Compute m_total_width
};

//////////////////////////////////////////////////////////////////

// Initializer, or "reset" method: write default values

template<class T,class A,bool bW,class W,bool bP,class P>
inline void
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  init_tree_fields ()                    // Write default values
{
  m_parent =
  m_children[L] = m_children[R] = NULL; // No relatives

  m_next = m_prev = this;     // Loop list
  m_height = m_count = 1;     // Single element, single level
}

// Initializer, or "reset" method: write default values

template<class T,class A,bool bW,class W,bool bP,class P>
inline void
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  init ()                                // Write default values
{                                        // Init NPSV width too
  init_tree_fields ();

  if (bW)
    *m_node_width = W(1);        // Default width
}

// Constructor: just call init()

template<class T,class A,bool bW,class W,bool bP,class P>
inline
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  avl_array_node_tree_fields ()
{ init (); }

// Helper functions: return count/height/width of left/right
// subtree. This doesn't require loops or recursion. If the
// left/right subtree is empty, return 0; otherwise, return
// the count/height/width of its root. Time required is O(1)

template<class T,class A,bool bW,class W,bool bP,class P>
inline std::size_t
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  left_count ()                               const
{
  return m_children[L] ?
         m_children[L]->m_count : 0;
}

template<class T,class A,bool bW,class W,bool bP,class P>
inline std::size_t
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  right_count ()                              const
{
  return m_children[R] ?
         m_children[R]->m_count : 0;
}

template<class T,class A,bool bW,class W,bool bP,class P>
inline std::size_t
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  left_height ()                              const
{
  return m_children[L] ?
         m_children[L]->m_height : 0;
}

template<class T,class A,bool bW,class W,bool bP,class P>
inline std::size_t
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  right_height ()                             const
{
  return m_children[R] ?
         m_children[R]->m_height : 0;
}

template<class T,class A,bool bW,class W,bool bP,class P>
inline void
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  get_left_width (W & w)                      const
{
  AA_ASSERT (bW);

  w = m_children[L] ?
      m_children[L]->total_width() : W(0);
}

template<class T,class A,bool bW,class W,bool bP,class P>
inline void
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  get_right_width (W & w)                     const
{
  AA_ASSERT (bW);

  w = m_children[R] ?
      m_children[R]->total_width() : W(0);
}

template<class T,class A,bool bW,class W,bool bP,class P>
inline void
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  update_width ()
{
  if (!bW)
    return;

  if (m_children[L] || m_children[R])
  {
    if (m_children[L])
    {
      *m_total_width = m_children[L]->total_width ();
      *m_total_width += *m_node_width;
    }
    else
      *m_total_width = *m_node_width;

    if (m_children[R])
      *m_total_width += m_children[R]->total_width ();
  }
}

template<class T,class A,bool bW,class W,bool bP,class P>
inline const W &
  avl_array_node_tree_fields<T,A,bW,W,bP,P>::
  total_width ()                        const
{
  AA_ASSERT (bW);

  if (bW)
    return (m_children[L] || m_children[R]) ? *m_total_width :
                                              *m_node_width;
  else
    return *(W*)NULL;
}

//////////////////////////////////////////////////////////////////

  }  // namespace detail

}  // namespace mkr

#ifdef AA_NO_ZERO_SIZE_ARRAYS
#undef AA_NO_ZERO_SIZE_ARRAYS
#endif

#endif

