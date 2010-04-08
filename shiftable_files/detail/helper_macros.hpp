///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  helper_macros.hpp
  -----------------
  
  Helper macros for code clarity (really!). Note that the C++
  object only contains a pointer to the memory region where
  the file is mapped. The fields of the header are _not_ part
  of the shiftable_file object.
*/

#ifndef _HELPER_MACROS_HPP_
#define _HELPER_MACROS_HPP_

#include "assert.hpp"

#define ROUND_TO_POW2_MULT(X,P) (((X)+(1<<(P))-1)&(~0UL<<P))

#define DUMMY       ((sizeof(header)+NODE_SIZE-1)>>LOG2_NODE_SIZE)
#define PDUMMY      ((node*)(m_pmap + (DUMMY<<LOG2_NODE_SIZE)))

#define PNODE(N)    (/*shf_assert((N)>=DUMMY),*/ \
                     (node*)(m_pmap + ((N)<<LOG2_NODE_SIZE)))

#define PHEADER     ((header*)m_pmap)
#define PBLOCK(N)   (m_pmap + ((N) << LOG2_BLOCK_SIZE))
#define PSWAP       (m_pmap + (PHEADER->m_map_size - BLOCK_SIZE))

#define PARENT(N)   (PNODE(N)->m_parent)
#define PPARENT(N)  (PNODE(PARENT(N)))
#define LEFT(N)     (PNODE(N)->child(L))
#define PLEFT(N)    (PNODE(LEFT(N)))
#define RIGHT(N)    (PNODE(N)->child(R))
#define PRIGHT(N)   (PNODE(RIGHT(N)))
#define CHILD(N,S)  (PNODE(N)->child(S))
#define PCHILD(N,S) (PNODE(CHILD((N),(S))))
#define NEXT(N)     (PNODE(N)->m_next)
#define PNEXT(N)    (PNODE(NEXT(N)))
#define PREV(N)     (PNODE(N)->m_prev)
#define PPREV(N)    (PNODE(PREV(N)))

#endif
