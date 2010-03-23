///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2006-2009, Universidad de Alcala               //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  detail/aa_npsv.hpp
  ------------------

  Methods for "Non-Proportional Sequence View" support:

  npsv_update_sums(): udate width sums O(1) or O(N)*
  npsv_width(): get total width O(1) or O(N)*
  npsv_width(it): get an element's width O(1)
  npsv_set_width(): set an element's width O(log N) or O(1)**
  npsv_pos_of(): get an element's position O(log N) or O(N)*
  npsv_at_pos(): get elem. of a position O(log N) or O(N)*
  npsv_insert(): insert and set width O(log N)
  (*) width sums need to be updated
  (**) don't update width sums (lazy mode)
*/

#ifndef _AVL_ARRAY_NON_PROPORTIONAL_SEQUENCE_VIEW_HPP_
#define _AVL_ARRAY_NON_PROPORTIONAL_SEQUENCE_VIEW_HPP_

#ifndef _AVL_ARRAY_HPP_
#error "Don't include this file. Include avl_array.hpp instead."
#endif

namespace mkr  // Public namespace
{

//////////////////////////////////////////////////////////////////

// ---------------------- PUBLIC INTERFACE -----------------------

// npsv_update_sums(): Update the m_total_width field of
// every node in the tree. This is achieved in O(N) time
// with a post-order traversal of the tree. This method
// will be called when, after changing NPSV widths in the
// lazy mode (see npsv_set_width()), they are required for
// any operation. If the lazy mode is not used, or if the
// sums are already up to date, then this method is a nop.
//
// Complexity: O(N)

template<class T,class A,bool bW,class W,bool bP,class P>
//not inline
  void
  avl_array<T,A,bW,W,bP,P>::npsv_update_sums (bool force) const
{
  AA_ASSERT (bW);

  if (!bW)
    return;

  node_t * p;

  if (!force && !m_sums_out_of_date)    // Already ok?
    return;                             // get out

  p = node_t::m_next;   // Go to leftmost node in the tree

  if (!p->m_parent)               // If the avl_array is empty
  {                               // just clear the
    m_sums_out_of_date = false;   // NPSV dirty flag
    return;                       // and get out
  }

  for (;;)
  {                                              // Go down as
    while (p->m_children[L] || p->m_children[R]) // deep as
      p = p->m_children[L] ?                     // possible
          p->m_children[L] : p->m_children[R];   // (preferably
                                                 // left)

    while (p->m_parent->m_children[R]==p ||
           !p->m_parent->m_children[R])          // If p is the
    {                                            // last child,
      p = p->m_parent;                           // update, go
                                                 // up and see
      p->update_width ();                        // again

      if (!p->m_parent)                // If we reached the
      {                                // root, it's done
        m_sums_out_of_date = false;
        return;
      }
    }
                                       // p is the left child,
    p = p->m_parent->m_children[R];    // go to the right one
  }
}

// npsv_width(): retrieve the total width of the array
// (the position of end in the alternative sequence (in
// which every node can occupy a different width instead
// of just one unit).
//
// Complexity: O(1), or O(N) if sums were not up to date

template<class T,class A,bool bW,class W,bool bP,class P>
inline
  const W & avl_array<T,A,bW,W,bP,P>::npsv_width () const
{
  AA_ASSERT (bW);

  if (!bW)
    return *(W*)0;

  if (m_sums_out_of_date)
    npsv_update_sums ();

  return node_t::total_width ();
}

// npsv_width(): retrieve the width of a node (the
// width is the amount of 'space' it occupies in the
// alternative sequence).
//
// Complexity: O(1)

template<class T,class A,bool bW,class W,bool bP,class P>
inline
  const W & avl_array<T,A,bW,W,bP,P>::npsv_width
  (typename avl_array<T,A,bW,W,bP,P>::const_iterator it) const
{
  AA_ASSERT (bW);
  AA_ASSERT (it.ptr);          // it must point somewhere

  return bW ? *it.ptr->m_node_width : *(W*)0;
}

// npsv_set_width(): modify the width of a node (the
// width is the amount of 'space' it occupies in the
// alternative sequence). If the third parameter is false,
// don't update width sums (it will be done later for the
// whole tree). This lazy technique will save time in
// those cases where many widths need to be updated in a
// row, getting O(N) complexity instead of O(n log N).
//
// Complexity: O(log N), or O(1) if update_sums==false

template<class T,class A,bool bW,class W,bool bP,class P>
//not inline
  void
  avl_array<T,A,bW,W,bP,P>::npsv_set_width
  (const typename avl_array<T,A,bW,W,bP,P>::iterator & it,
   const W & w,
   bool update_sums)
{
  AA_ASSERT (bW);

  if (!bW)
    return;

  node_t * p;

  AA_ASSERT (it.ptr);                 // it must point somewhere
  AA_ASSERT_HO (owner(it.ptr)==this); // it must point here

  AA_ASSERT_EXC (it.ptr->m_parent,       // Can't change
                 invalid_op_with_end()); // end's width

  *it.ptr->m_node_width = w;    // Set the new width

  if (update_sums)              // If required, update all sums
  {                             // of the tree, or just climb
    if (m_sums_out_of_date)     // to the root updating sums
      npsv_update_sums ();      // in the way
    else
      for (p=it.ptr; p; p=p->m_parent)
        p->update_width ();
  }
  else                          // If no sums update,
    m_sums_out_of_date = true;  // set NPSV dirty bit
}

// npsv_pos_of(): given a node, calculate its position
// in the alternative sequence (in which every node can
// occupy a different width instead of just one unit).
//
// Complexity: O(log N), or O(N) if sums are out of date

template<class T,class A,bool bW,class W,bool bP,class P>
//not inline
  W
  avl_array<T,A,bW,W,bP,P>::npsv_pos_of
  (typename avl_array<T,A,bW,W,bP,P>::const_iterator it) const
{
  AA_ASSERT (bW);

  if (!bW)
    return W(0);

  W pos, w;
  const node_t * p, * parent;

  p = it.ptr;

  AA_ASSERT (p);  // NULL pointer dereference

  if (m_sums_out_of_date)
    npsv_update_sums ();

  if (!p->m_parent)               // Already in the dummy node?
    return node_t::total_width ();
                                  // Otherwise, start with the
  for (p->get_left_width(pos);    // left width of the node and
       p->m_parent;               // climb up to the root adding
       p=parent)                  // the widths of nodes that stay
  {                               // in the left of the path
    parent = p->m_parent;
                                        // That is, when we
    if (parent->m_children[R]==p)       // step up-left, the
    {                                   // width of the parent
      if (parent->m_children[L])        // node and its left
      {                                 // width
        parent->get_left_width (w);
        w += *parent->m_node_width;
      }
      else
        w = *parent->m_node_width;

      w += pos;
      pos = w;
    }
  }

  return pos;
}

// npsv_at_pos (): find a node, given its position in the
// alternative sequence (in which every node can occupy a
// different width instead of just one unit).
// It travels down from the root to the searched node. This
// takes logarithmic time both on average and in the worst
// case.
//
// Complexity: O(log N), or O(N) if sums are out of date

template<class T,class A,bool bW,class W,bool bP,class P>
//not inline
  typename avl_array<T,A,bW,W,bP,P>::iterator
  avl_array<T,A,bW,W,bP,P>::npsv_at_pos
  (W pos, bool first)
{
  AA_ASSERT (bW);

  if (!bW)
    return typename my_class::iterator();

  node_t * p;
  W w, left, right, offset(0);

  if (m_sums_out_of_date)
    npsv_update_sums ();

  if (size()==0 || pos<W(0) ||
      node_t::total_width()<pos ||      // Out of bounds --> end
      (pos==node_t::total_width() &&
       !(*node_t::m_prev->m_node_width==W(0))))
    return dummy ();

  p = node_t::m_children[L]; // Start with the element at the
                             // root (remember that the dummy
                             // node has no T element)

      // For every subtree, the position (in it) of its root
      // node is equal to the left width of this root node...

      // Descend through the tree, binary-searching for the
      // desired position. The value of 'offset' is the position
      // of the current subtree's leftmost node with respect to
      // the beginning of the sequence W(0). 'left' and 'right'
      // represent the sides of the current node

      // Note that there might be nodes with zero width (some
      // nodes standing in the same position of the alternative
      // view). In these cases return the first one or the last
      // one, according to the value of the parameter 'first'

  while (p)
  {
    left = offset;
    p->get_left_width (w);
    left += w;

    right = left;
    right += *p->m_node_width;

    if (pos<left ||
        (p->m_children[L] &&
         first && pos==left &&
         *p->m_prev->m_node_width==W(0)))
      p = p->m_children[L];
    else if (pos<right ||
             (first && pos==right &&
              *p->m_node_width==W(0)))
      return p;
    else
    {
      offset = right;
      p = p->m_children[R];
    }
  }

                   // We sould never step out of the tree, but
  return dummy (); // don't trust the coherency of W and its
}                  // operators (just in case)

// npsv_at_pos() _const_: See non-const version (above) for
// details.

template<class T,class A,bool bW,class W,bool bP,class P>
//not inline
  typename avl_array<T,A,bW,W,bP,P>::const_iterator
  avl_array<T,A,bW,W,bP,P>::npsv_at_pos
  (W pos, bool first)                   const
{
  AA_ASSERT (bW);
  return bW ? (const_cast<my_class*>(this))->
                            npsv_at_pos (pos, first) :
              typename my_class::const_iterator();
}

// npsv_at_pos (): find a node, given its position in the
// alternative sequence (in which every node can occupy a
// different width instead of just one unit).
// Use a functor (passed as parameter) instead of the usual
// operators <, >, ==, !=, <=, >=.
// This version of the method is intended for using a
// composite width type. The functor parameter cmp must
// have an overloaded operator() which receives two W
// const references, and returns an integer indicating
// the result of the comparison (<0: first is lesser;
// 0: equal; >0: first is greater).
//
// Complexity: O(log N), or O(N) if sums are out of date

template<class T,class A,bool bW,class W,bool bP,class P>
template<class CMP>
//not inline
  typename avl_array<T,A,bW,W,bP,P>::iterator
  avl_array<T,A,bW,W,bP,P>::npsv_at_pos
  (W pos, CMP cmp, bool first)
{
#ifdef BOOST_CLASS_REQUIRE
  function_requires<
      BinaryFunctionConcept<CMP,int,const W&,const W&> >();
#endif

  AA_ASSERT (bW);

  if (!bW)
    return typename my_class::iterator();

  node_t * p;                   // See comments in the first
  W w, left, right, offset(0);  // version of this method (without
  int c;                        // cmp). The algorithm used here
                                // is the same. The only
  if (m_sums_out_of_date)       // difference is the comparison
    npsv_update_sums ();        // method.

  if (size()==0 ||
      cmp(pos,W(0))<0 ||                            // pos<(W)0
      (c=cmp(pos,node_t::total_width()))>0 ||       //  " >total_w
      (c==0 &&                                      //  " == "
       cmp(*node_t::m_prev->m_node_width,W(0))!=0)) // prev_w != 0
    return dummy ();

  p = node_t::m_children[L];

  while (p)
  {
    left = offset;
    p->get_left_width (w);
    left += w;

    right = left;
    right += *p->m_node_width;

    if ((c=cmp(pos,left))<0 ||               // pos < left
        (p->m_children[L] &&
         first &&
         c==0 &&                             // pos == left
         cmp(*p->m_prev->m_node_width,W(0))
                                       ==0)) // prev_w == 0
      p = p->m_children[L];
    else if ((c=cmp(pos,right))<0 ||         // pos < right
             (first &&
              c==0 &&                        // pos == right
              cmp(*p->m_node_width,W(0))
                                       ==0)) // p_w == 0
      return p;
    else
    {
      offset = right;
      p = p->m_children[R];
    }
  }

  return dummy ();
}

// npsv_at_pos() _const_: See non-const version (above) for
// details.

template<class T,class A,bool bW,class W,bool bP,class P>
template<class CMP>
inline
  typename avl_array<T,A,bW,W,bP,P>::const_iterator
  avl_array<T,A,bW,W,bP,P>::npsv_at_pos
  (W pos, CMP cmp, bool first)          const
{
#ifdef BOOST_CLASS_REQUIRE
  function_requires<
      BinaryFunctionConcept<CMP,int,const W&,const W&> >();
#endif

  AA_ASSERT (bW);
  return bW ? (const_cast<my_class*>(this))->
                            npsv_at_pos (pos, cmp, first) :
              typename my_class::const_iterator();
}

// Insert Before and assign width: insert a new T copy
// constructed element before a given position. Return iterator
// pointing to the new element
//
// Complexity: O(log(N))

template<class T,class A,bool bW,class W,bool bP,class P>
inline
  typename avl_array<T,A,bW,W,bP,P>::iterator
  avl_array<T,A,bW,W,bP,P>::npsv_insert
  (const typename avl_array<T,A,bW,W,bP,P>::iterator & it, // Where
   typename avl_array<T,A,bW,W,bP,P>::const_reference t,   // What
   const W & w)                                            // Width
{
  node_t * newnode;

  AA_ASSERT (bW);
  newnode = new_node (&t);

  if (bW)
    *newnode->m_node_width = w;

  insert_before (newnode, it.ptr);

  return iterator(newnode);
}

//////////////////////////////////////////////////////////////////

}  // namespace mkr

#endif

