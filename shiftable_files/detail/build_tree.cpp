///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  build_tree.cpp
  --------------
  
  Balanced tree construction in O(N) time.
*/

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;


/*  -------------------------------------------------------
   worth_rebuild()

   Decide whether a massive operation is worth rebuilding the
   whole tree (O(N)), or it is better to insert/extract the
   nodes one by one (O(n log N)).
*/

bool shiftable_file::worth_rebuild (uint32_t diff_nodes,
                                    bool erase) const
{
  uint32_t average_size, final_size, ratio, n, N;

  n = diff_nodes; // Nodes to insert/erase

  N = ((PHEADER->m_map_size -          // Current size
        PHEADER->m_meta_data_size) >>
       LOG2_BLOCK_SIZE                ) -
      (1 + PHEADER->m_free_count);

  if (n<=1)               // We need to choose between
    return false;         // O(n log N) and O(N), or more
                          // exactly: O(n log(average_size))
                          // and O(final_size). If the latter
  if (erase)              // is lesser, it is worth rebuilding
  {                       // the whole tree.
    shf_assert (N>=n);
                            // Demo:
    average_size = N - n/2; //
    final_size = N - n;     // n log2(aver) >    final
  }                         //              "
  else                      //  log2(aver)  >   final/n
  {                         //              "
    average_size = N + n/2; //     aver     >  pow(2,final/n)
    final_size = N + n;     //              "
  }                         //     aver     > (1 << (final/n))

  ratio = (final_size + n/2) / n;   // (n/2)/n==.5 for rounding
                                    // the integer division
  if (ratio >= sizeof(uint32_t)*8)
    return false;                   // Avoid << overflow
  else
    return average_size >           // O(n log N) > O(N) ?
           (1U<<ratio);
}


/*  -------------------------------------------------------
    build_tree()

    Build a new tree of a given number of nodes, populating it
    with existing nodes taken from a list.

    This takes linear time. That is, proportional to the
    number of nodes. The tree is directly built in perfect
    balance, while traversed in-order. Two stack arrays of
    fixed size (one element per bit in an integer) are used
    for making this in-order travel thorugh the tree under
    construction. No function recursion is used. Can it be
    more efficient?
*/

uint32_t shiftable_file::build_tree (uint32_t first,
                                     uint32_t num)
{
  uint32_t depth;           // Current depth
  uint32_t cur, last, next; // Current and last nodes
  node * pcur, * plast;     // (numbers and pointers)
  uint32_t left;
  node * pleft, * pparent;

  uint32_t                  // Per level: number of nodes
    counts[8*sizeof         // that still have to be created
             (uint32_t)];   // in the subtree

  uint32_t                  // Per level...
    nodes[8*sizeof          // a) above depth: 0 or already
            (uint32_t)];    //    existing parent of a subtree
                            //    under creation
                            // b) bellow depth: 0 or already
                            //    existing nodes that have to
                            //    be linked to their parent
                            //    (still not created)
  PDUMMY->init ();
  PDUMMY->m_next = DUMMY;
  PDUMMY->m_prev = DUMMY;

  if (num==0)
    return first;

  for (depth=0; depth<8*sizeof(uint32_t); depth++) // Clear
    nodes[depth] = 0;                              // stack

  counts[0] = num;    // Total number of nodes
  depth = 0;

  last = DUMMY;    // The first node will be linked to dummy
  plast = PDUMMY;
  next = first;

  for (;;)
  {
    while (counts[depth]>1 &&  // If we visit a subtree for the
           nodes[depth+1]==0)  // first time, go down-left to
    {                          // the leftmost position, where
      depth ++;                              // we will place
      counts[depth] = counts[depth-1] >> 1;  // the first node
      counts[depth-1] -= counts[depth];      // Half the count
    }                                        // on every step

    shf_assert (next>DUMMY);       // Enough data?

    cur = next;                    // Grab the next node
    pcur = PNODE(cur);

    next = pcur->m_next;           // Advance

    pcur->init (pcur->m_bytes);    // Clear the node

    pcur->m_prev = last;           // Insert the node after
    pcur->m_next = plast->m_next;  // the last one in the
    NEXT(pcur->m_prev) = cur;      // circular doubly linked
    PREV(pcur->m_next) = cur;      // list

    last = cur;                // The last one is now the one
    plast = pcur;              // we've just inserted

    nodes[depth] = cur;        // Put it in the stack
    counts[depth] --;          // One node less to go

    if (nodes[depth+1]>0)            // If there was something
    {                                // in the next level, it
      left = nodes[depth+1];         // is the left subtree
      pleft = PNODE(left);

      pleft->m_parent = cur;         // Link it
      pcur->child(L) = left;
      pcur->m_height += pleft->m_height;
      pcur->m_bytes_subtree += pleft->m_bytes_subtree;

      nodes[depth+1] = 0;            // Forget it
    }

    if (counts[depth]>0)       // More nodes to add here?
    {
      depth ++;                         // They will go on the
      counts[depth] = counts[depth-1];  // right subtree, so
      counts[depth-1] = 0;              // go down
    }
    else                       // Current subtree is done
      while (counts[depth]==0) // Go up to a place where more
      {                        // nodes need to be added and
        cur = nodes[depth];    // link the subtrees in the way
        pcur = PNODE(cur);

        if (depth==0)          // Back in the top level?
        {                             // Finished!
          pcur->m_parent = DUMMY;     // Link tree to dummy
          PDUMMY->child(L) = cur;
          PDUMMY->m_height = pcur->m_height + 1;
          PDUMMY->m_bytes_subtree = pcur->m_bytes_subtree;

          return next;  // Return remaining nodes of the list
        }

        depth --;              // Step up

        if (nodes[depth]>0)       // If there's a node there,
        {                         // what we have just built is
          nodes[depth+1] = 0;             // its right subtree
          pcur->m_parent = nodes[depth];
                                          // Link it
          pparent = PNODE(nodes[depth]);
          pparent->m_bytes_subtree += pcur->m_bytes_subtree;
          pparent->child(R) = cur;
        }                         // (the height is already
      }                           // ok because the left subtree
  }                               // is allways >= the right one)
}

