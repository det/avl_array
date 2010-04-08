///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  node.hpp
  --------
  
  Definition of the nodes of the tree.
*/

#ifndef _SHF_NODE_HPP_
#define _SHF_NODE_HPP_

#include <memory.h>
#include "../shiftable_files.hpp"

#ifdef __GNUC__
#define GCC_ATTR_PACKED __attribute__((__packed__))
#else
#define GCC_ATTR_PACKED
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

#define LOG2_NODE_SIZE 5               // pow (2, 5) ==
#define NODE_SIZE (1<<LOG2_NODE_SIZE)  //               32 bytes

namespace shiftable_files
{

namespace detail
{

typedef enum { L, R } left_right;

class node                 // 8 unsigned integers of
{                          // of 4 bytes each ... total: 32 bytes
  private:

    uint32_t m_children[2];  // Children nodes in the tree,
                             // prev an next when free
  public:

    uint32_t & child (int side)
    { return m_children[side]; }

    uint32_t & left () { return child(L); }
    uint32_t & right () { return child(R); }

    uint32_t & prev_free () { return child(L); }
    uint32_t & next_free () { return child(R); }

    uint32_t m_parent;           // Parent node in the tree

    uint32_t m_prev;             // Prev and next nodes in the
    uint32_t m_next;             // sequence, 0 when free

    uint32_t m_bytes;            // Data bytes in the node
    uint32_t m_bytes_subtree;    // Data bytes in the subtree

    uint32_t m_height;           // Height of the subtree

    void init (uint32_t bytes=0)
    {
      memset (this, 0, sizeof(*this));
      m_bytes = m_bytes_subtree = bytes;
      m_height = 1;
    }

    bool is_free () const
    { return m_prev==0 && m_next==0; }

    void free ()
    { m_prev = m_next = 0; }
}
GCC_ATTR_PACKED;

#if LOG2_NODE_SIZE >= LOG2_BLOCK_SIZE
#error "Blocks _must_ be larger than nodes"
#endif

           // sizeof() can't be used in the preprocessor stage,
           // so the next dummy type will do the trick:
typedef
char Nodes_are_larger_than_dictated_by_LOG2_NODE_SIZE
[2*((    sizeof(node) > (1<<LOG2_NODE_SIZE)    )==0)-1];

} // namespace detail

} // namespace shiftable_files

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#undef GCC_ATTR_PACKED

#endif
