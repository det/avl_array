///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  check_integrity.cpp
  -------------------
  
  Verify the correctness of a shiftable file, including the header
  and the invariants of the AVL balanced tree.
  
  This code should be compiled only in the DEBUG version of the
  library. It makes shiftable_files very very sloooow.
  
  If you are compiling the library with your program, and you
  want to debug your program, but _not_ the library, retouch the
  DEBUG-related directives below. Remember to change accordingly
  the directives at the end of the class declaration in
  shiftable_files.hpp.
  
  // TODO: Generalize for using it in a 'safe' open() and comment
*/

#include "types.hpp"

#if defined(_DEBUG) || defined(DEBUG)

#include <iostream>
#include <vector>

#include "assert.hpp"

#include "../shiftable_files.hpp"
#include "header.hpp"
#include "node.hpp"
#include "helper_macros.hpp"

using namespace shiftable_files;
using namespace shiftable_files::detail;

#define CHECK(X,M) SENTENCE (                    \
  if(!(X))                                       \
  {                                              \
    std::cerr << std::endl                       \
         << __LINE__ << ": " << #X << std::endl  \
         << "ERROR: " << M << std::endl;         \
    std::cerr.flush ();                          \
    STOP;                                        \
  }                                              \
)

bool shiftable_file::check_integrity (bool check_pos) const
{
  header * h;
  node * n, * dummy;
  uint32_t i;
  uint32_t map_size, meta_data_size, total_blocks;
  uint32_t unusable_nodes, usable_nodes;
  uint32_t free_count, bytes_count;
  uint32_t cur, prev;
  uint32_t first_node, num_nodes;

  if (!is_open())
    return true;

  std::cout.flush ();

  h = PHEADER;

  CHECK (h->check_magic_bytes(), "Invalid file signature");
  CHECK (h->verify_compatibility(), "Incompatible file format");

  map_size = h->m_map_size;
  meta_data_size = h->m_meta_data_size;

  CHECK ((map_size&(BLOCK_SIZE-1))==0, "Unaligned map size");
  CHECK ((meta_data_size&(BLOCK_SIZE-1))==0, "Unaligned metadata size");

  CHECK (map_size>meta_data_size, "Wrong sizes");

  total_blocks = ( (map_size -
                    meta_data_size) >> LOG2_BLOCK_SIZE ) - 1;

  CHECK (total_blocks>0, "Wrong sizes");
  CHECK (total_blocks<=MAX_BLOCKS, "Wrong sizes");

  unusable_nodes = DUMMY + 1;

  if (unusable_nodes < (meta_data_size >> LOG2_BLOCK_SIZE))
    unusable_nodes = meta_data_size >> LOG2_BLOCK_SIZE;

  usable_nodes = (meta_data_size >> LOG2_NODE_SIZE) -
                 unusable_nodes;

  CHECK (usable_nodes>=total_blocks, "Wrong sizes");

  std::vector<bool> flags(map_size>>LOG2_BLOCK_SIZE);
  std::vector<bool>::iterator f;

  first_node = unusable_nodes;
  num_nodes = total_blocks;

  for (i=first_node, free_count=bytes_count=0;
       i<first_node+num_nodes;
       i++)
    if (PNODE(i)->is_free())
      free_count ++;
    else
    {
      n = PNODE(i);
      CHECK (n->m_bytes>0, "Occupied block has 0 bytes");
      CHECK (n->m_bytes<=BLOCK_SIZE, "Occupied block has " <<
                                     n->m_bytes << " bytes (too many)");
      bytes_count += n->m_bytes;
    }

  CHECK (bytes_count==PDUMMY->m_bytes_subtree, "Wrong bytes count");
  CHECK (free_count==h->m_free_count, "Wrong free nodes count");

  CHECK ((h->m_free_list_first!=0) == (h->m_free_list_last!=0),
         "Inconsistent free list");

  CHECK ((h->m_free_list_first!=0) == (free_count!=0),
         "Inconsistent free list");

  // Check free nodes list

  if (free_count)
  {
    CHECK (PREV(h->m_free_list_first)==0, "Broken free list");
    CHECK (NEXT(h->m_free_list_last)==0, "Broken free list");

    for (f=flags.begin(); f!=flags.end(); ++f)
      *f = false;

    prev = i = 0;
    cur = h->m_free_list_first;

    do
    {
      CHECK (cur>=first_node && cur<first_node+num_nodes,
             "Free node number is out of range");

      n = PNODE(cur);

      CHECK (n->is_free(),
             "Occupied node appears in free list");

      CHECK (!flags[cur], "Loop in free list");
      flags[cur] = true;

      CHECK (n->prev_free()==prev, "Broken free list");

      prev = cur;
      cur = n->next_free();
      i ++;
    }
    while (cur);

    CHECK (i==free_count, "Free list is too long/short");
  }

  // Check tree

  dummy = PDUMMY;

  CHECK ((dummy->m_next==DUMMY) == (free_count==total_blocks),
         "Inconsistent dummy node and/or free nodes count");

  CHECK ((dummy->m_next==DUMMY) == (dummy->m_prev==DUMMY),
         "Inconsistent dummy node");

  CHECK ((dummy->m_next==DUMMY) == (dummy->left()==0),
         "Inconsistent dummy node");

  CHECK (dummy->right() == 0,
         "Inconsistent dummy node");

  if (!dummy->left())  // If empty...
  {
    if (check_pos)
      CHECK (m_cur_node==DUMMY && m_rel_pos==m_abs_pos,
             "Inconsistent position");

    return true;
  }

  // First, check the list of occupied nodes

  for (f=flags.begin(); f!=flags.end(); ++f)
    *f = false;

  i = 0;
  prev = DUMMY;
  cur = dummy->m_next;

  do
  {
    CHECK (cur>=first_node && cur<first_node+num_nodes,
           "Occupied node number is out of range");

    n = PNODE(cur);

    CHECK (!n->is_free(),
           "Free node appears in occupied list");

    CHECK (!flags[cur], "Loop in occupied list");
    flags[cur] = true;

    CHECK (n->m_prev==prev, "Broken occupied list");

    prev = cur;
    cur = n->m_next;
    i ++;
  }
  while (cur!=DUMMY);

  CHECK (i+free_count<=total_blocks, "Impossible nodes count");
  CHECK (i+free_count==total_blocks, "Lost nodes");

  // Second, check that the tree is a proper tree

  for (f=flags.begin(); f!=flags.end(); ++f)
    *f = false;

  if (!check_subtree(DUMMY,flags,first_node,num_nodes))
    return false;

  flags[DUMMY] = false;

  for (f=flags.begin(), i=0; f!=flags.end(); ++f)
    if (*f)
      i ++;

  CHECK (i+free_count<=total_blocks, "Impossible nodes count");
  CHECK (i+free_count==total_blocks, "Lost nodes");

  // Third, check consistence list-tree

  cur = PDUMMY->m_next;
  CHECK (LEFT(cur)==0, "First node is not the leftmost leaf");

  while (cur!=DUMMY)
  {
    prev = cur;

    if (RIGHT(cur))
    {
      cur = RIGHT(cur);

      while (LEFT(cur))
        cur = LEFT(cur);
    }
    else
    {
      while (RIGHT(PARENT(cur))==cur)
        cur = PARENT(cur);

      cur = PARENT(cur);
    }

    CHECK (NEXT(prev)==cur,
           "Occupied list doesn't match inorder tree walk");
  }

  // TODO: Fourth, check fragmentation limits?

  // Finally, check position

  if (check_pos)
  {
    if (m_cur_node==DUMMY)
      CHECK (dummy->m_bytes_subtree+m_rel_pos == m_abs_pos,
             "Inconsistent position");
    else
    {
      uint32_t rel_pos, cur_node;

      CHECK (m_cur_node >= unusable_nodes,
             "Current node is out of range");

      CHECK (m_cur_node < unusable_nodes+total_blocks,
             "Current node is out of range");

      CHECK (!PNODE(m_cur_node)->is_free(),
             "Current node is a free node");

      CHECK (m_rel_pos<=PNODE(m_cur_node)->m_bytes,
             "Relative position in current node is out of range");

      find_pos (m_abs_pos, cur_node, rel_pos);

      if (m_rel_pos==0 &&
          rel_pos==PNODE(cur_node)->m_bytes)
      {
        cur_node = NEXT(cur_node);
        rel_pos = 0;
      }
      else if (m_rel_pos==PNODE(m_cur_node)->m_bytes &&
               rel_pos==0)
      {
        cur_node = PREV(cur_node);
        rel_pos = PNODE(cur_node)->m_bytes;
      }

      CHECK (m_cur_node==cur_node && m_rel_pos==rel_pos,
             "Current node and relative position "
             "don't match absolute position");
    }
  }

  return true;
}

bool shiftable_file::check_subtree (uint32_t cur,
                                    std::vector<bool> & flags,
                                    uint32_t first,
                                    uint32_t num) const
{
  uint32_t left, right, height, bytes;
  uint32_t lh, rh, lb, rb;
  int balance;
  node * n;

  if (!cur)
    return true;

  n = PNODE(cur);

  if (cur==DUMMY)
  {
    CHECK (n->m_bytes==0, "Dummy node seems to have data");
    CHECK (n->m_parent==0, "Dummy node seems to have a parent");
  }
  else
    CHECK (!n->is_free(), "Free node appears in the tree");

  CHECK (flags[cur]==false, "Loop in the tree");
  flags[cur] = true;

  left = n->left();
  right = n->right();

  // Check children indexes

  if (left)
    CHECK (left>=first && left<first+num,
           "Left child number is out of range");

  if (right)
    CHECK (right>=first && right<first+num,
           "Right child number is out of range");

  if (left && right)
    CHECK (left!=right, "Left child and right child are the same node");

  // Check that my children haven't been visited

  CHECK (flags[left]==false, "Loop in the tree (left child)");
  CHECK (flags[right]==false, "Loop in the tree (right child)");

  // Check that I am my children's parent

  if (left)
    CHECK (PARENT(left)==cur,
           "Broken parent link (left child)");

  if (right)
    CHECK (PARENT(right)==cur,
           "Broken parent link (right child)");

  // Check height and sum of bytes

  if (left)
  {
    lh = PNODE(left)->m_height;
    lb = PNODE(left)->m_bytes_subtree;
  }
  else
    lh = lb = 0;

  if (right)
  {
    rh = PNODE(right)->m_height;
    rb = PNODE(right)->m_bytes_subtree;
  }
  else
    rh = rb = 0;

  height = 1 + (lh>rh ? lh : rh);
  bytes = lb + n->m_bytes + rb;

  CHECK (n->m_bytes_subtree==bytes, "Wrong subtree bytes count");
  CHECK (n->m_height==height, "Wrong subtree height");

  if (cur!=DUMMY)
  {
    balance = (int)rh - (int)lh;
    CHECK (balance>=-1 && balance<=1, "Wrong balance");
  }

  return check_subtree (n->left(), flags, first, num) &&
         check_subtree (n->right(), flags, first, num);
}

#endif // defined(_DEBUG) || defined(DEBUG)
