///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  balance.cpp
  -----------
  
  Helper methods for maintaining the tree balance.
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;


/*  -------------------------------------------------------
   update_counters()

   Climb from a node to the root updating all height and
   bytes count fields in the way
*/

void shiftable_file::update_counters (uint32_t n)
{
  uint32_t i, j, left, right;
  node * p;

  while (n)                      // Climb until the root is
  {                              // reached
    p = PNODE(n);

    left = p->left ();
    right = p->right ();

    i = left ? PNODE(left)->m_height : 0;
    j = right ? PNODE(right)->m_height : 0;

    p->m_height = (i>j?i:j) + 1; // The height is that of the
                                 // largest branch, plus one

    i = left ? PNODE(left)->m_bytes_subtree : 0;
    j = right ? PNODE(right)->m_bytes_subtree : 0;

    p->m_bytes_subtree =         // The sum of bytes is the
            i + j + p->m_bytes;  // sum of the subtrees' sums
                                 // plus the node's bytes

    n = p->m_parent;             // Step up
  }
}


/*  -------------------------------------------------------
    update_counters_and_rebalance()

    Climb from a node to the root updating all height and
    bytes count fields in the way _and_ rebalancing with AVL
    rotations if necessary (this is what makes the whole thing
    possible, so thanks go to G.M. Adelson-Velsky and E.M.
    Landis)
*/

void shiftable_file::update_counters_and_rebalance (uint32_t n)
{
  uint32_t i, j, left, right, nq, nr;
  int s;
  node * p, * q, * r;

  while (n)                      // Climb until the root is
  {                              // reached
    p = PNODE(n);

    left = p->left ();
    right = p->right ();

    i = left ? PNODE(left)->m_height : 0;
    j = right ? PNODE(right)->m_height : 0;

    p->m_height = (i>j?i:j) + 1; // The height is that of the
                                 // largest branch, plus one

    s = -1; // -1 means balanced

    if (p->m_parent)  // (don't re-balance dummy node)
    {
      if (i > j+1)       // Unbalanced to the left..
        s = R;           // .. rotate to the right
      else if (j > i+1)  // Unbalanced to the right..
        s = L;           // .. rotate to the left
    }
                             // If we are in the dummy node or
    if (s==-1)               // the current node is balanced
    {
      i = left ? PNODE(left)->m_bytes_subtree : 0;
      j = right ? PNODE(right)->m_bytes_subtree : 0;

      p->m_bytes_subtree =       // The sum of bytes is the
            i + j + p->m_bytes;  // sum of the subtrees' sums
                                 // plus the node's bytes

      n = p->m_parent;           // Step up
      continue;                  // This node is done
    }

                             // Otherwise... re-balance!
    n = p->child (1-s);      // Go down to child
    p = PNODE(n);            // (side of the long branch)

    left = p->left ();
    right = p->right ();                  // Calculate heights
                                           // of this new
    i = left ? PNODE(left)->m_height : 0;   // position
    j = right ? PNODE(right)->m_height : 0;

         // Consider the sub-subtrees of the long subtree.
         // If the sub-subtree in the middle (between the
         // other sub-subtree and the short subtree) is shorter
         // or equal to the side sub-subtree, we just need
         // a simple rotation; otherwise, we need a double
         // rotation

    if ( (s==R && i>=j) ||      // If a simple rotation
         (s==L && i<=j)    )    // is enough...
    {
      /*
        Before:       C
                      |              Y shorter or eq. to X
                      B
                   /     \                   n,p -> A
                A         Z
              /   \      ***
             X     Y    *****_____
            ***   ***              |
           ***** *****_ _          | 2 <-- unbalanced!
           ***** .....    | 0/1    |
           *****_.....____|_______ |

                                        Simple rotation
        After:          C               (with s==R in
                        |               the figure)
                        A
                     /     \
                   X         B
                  ***      /   \
                 *****    Y     Z
                 *****   ***   ***
              ___*****  ***** *****___
        0/1 |           .....          | 0/1
            | __________....._________ |


        Notation for comments in the code bellow:

          F
          |   F's downling points to G
          G   G's uplink points to F

          F
          v   F's downling points to G
          G   G's uplink points to somthing else (not to F)

          F
          ^   F's downling points to somthing else (not to G)
          G   G's uplink points to F
      */

      CHILD(p->m_parent,1-s) =
                          p->child (s);    //   B       B
                                           //  /  =>>  /^
      if (p->child(s))                     // A       Y A
        PARENT(p->child(s)) = p->m_parent;

                                           //  C       C
      p->child (s) = p->m_parent;          //  |       ^
      p->m_parent = PARENT(p->child(s));   //  B  =>>  A
      PARENT(p->child(s)) = n;             //  ^        \.
                                           //  A         B

      if (LEFT(p->m_parent)==p->child(s))  //  C       C
        LEFT(p->m_parent) = n;             //  v  =>>  |
      else                                 //  B       A
        RIGHT(p->m_parent) = n;

                             // Step down to B
      n = p->child (s);      // (it is balanced but it needs
    }                        // count and height updates)
    else
    {                           // If a double rotation
      nq = p->child (s);        // is required...
      nr = p->m_parent;

      q = PNODE(nq);
      r = PNODE(nr);

      /*
        Before:              C
                             |          Y larger than X
                             B
                          /     \            n,p -> A
                       A           Z         nq,q -> Y
                     /   \        ***        nr,r -> B
                   X       Y     *****___
                  ***     / \             |
              ___*****   U   V            | 2 <-- unbalanced!
          1 |           *** ***           |
            | _________**** ****_________ |


                                        Double rotation
        After:             C            (with s==R in
                           |            the figure)
                           Y
                        /     \
                     A           B
                   /   \       /   \
                  X     U     V     Z
                 ***   ***   ***   ***
                ***** ***** ***** *****

        (See notation for comments in simple rotation)
      */
                                             //   C
      q->m_parent = r->m_parent;             //   |
                                             //   B      C
      if (LEFT(q->m_parent) == nr)           //  /  =>>  |
        LEFT(q->m_parent) = nq;              // A        Y
      else                                   //  \.
        RIGHT(q->m_parent) = nq;             //   Y

      r->child (1-s) = q->child (s);         //   B       B
                                             //  /  =>>  /
      if (r->child(1-s))                     // A       V
        PARENT(r->child(1-s)) = nr;

      p->child (s) = q->child (1-s);         // A       A
                                             //  \  =>>  \.
      if (p->child(s))                       //   Y       U
        PARENT(p->child(s)) = n;
                                             // B  Y        Y
      q->child(1-s) = n;                     // ^  v  =>>  /
      p->m_parent = nq;                      // A  U      A

      q->child(s) = nr;                      // C  Y      Y
      r->m_parent = nq;                      // ^  v  =>>  \.
                                             // B  V        B

                   // A, B, Y, C and upper nodes need height
                   // and count updates

                   // Update B here and go on from A (and up)
                   // in the next iteration

      left = r->left ();
      right = r->right ();

      i = left ? PNODE(left)->m_height : 0;
      j = right ? PNODE(right)->m_height : 0;

      r->m_height = (i>j?i:j) + 1;

      i = left ? PNODE(left)->m_bytes_subtree : 0;
      j = right ? PNODE(right)->m_bytes_subtree : 0;

      r->m_bytes_subtree = i + j + r->m_bytes;
    }
  }
}

