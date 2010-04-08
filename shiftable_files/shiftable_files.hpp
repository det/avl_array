///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  shiftable_files.hpp
  -------------------
  
  Declaration of the class shiftable_file. Objects of this class
  represent ("shiftable") open files where the user (programmer)
  can read/write, seek (random access), and... insert/remove
  bytes.
  
  Usage example:
  
        #include "shiftable_files.hpp"
        using namespace shiftable_files;

        int main ()
        {
          shiftable_file shf;

          shf.open ("test.txt", om_create_or_wipe_contents);
          shf.write ("This is not a simple file.", 26);
          shf.seek_set (7);
          shf.remove (4);     // Delete " not"
          shf.close ();

          return 0;
        }
*/

#ifndef _SHIFTABLE_FILES_HPP_
#define _SHIFTABLE_FILES_HPP_

#include "detail/types.hpp"

#if defined(_DEBUG) || defined(DEBUG)
#include <vector>
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef LOG2_BLOCK_SIZE
#define LOG2_BLOCK_SIZE 10
#endif
#define BLOCK_SIZE (1<<LOG2_BLOCK_SIZE)

#define EXTRA_GROWTH(X) ((X)*3U/2U)
#define WORTH_SHRINK(OCCUPIED,TOTAL) ((OCCUPIED)<((TOTAL)/4U))

#if WORTH_SHRINK (100, EXTRA_GROWTH(100))
#error Inconsistent grow-shrink criterions
#endif

// The NEVER_DEFRAG_ON_SHRINK macro is optional.
// Define it for realtime applications. Comment it out
// for saving some disk space by compacting the data
// whenever a shrink operation (provoked by extract or
// resize) needs it.
//#define NEVER_DEFRAG_ON_SHRINK

#define COMPACTION_STEPS 2

#define MAX_SIZE ((~0U<<LOG2_BLOCK_SIZE)&\
                  (~0U/(COMPACTION_STEPS+1)*COMPACTION_STEPS))
#define MAX_BLOCKS (MAX_SIZE>>LOG2_BLOCK_SIZE)


namespace shiftable_files
{

typedef enum
{
  ft_true_file, ft_virtual_file
}
file_type;

typedef enum
{
  ff_plain_file, ff_shiftable_file, ff_auto_detect
}
file_format;

typedef enum
{
  om_create_or_wipe_contents, om_open_existing_or_fail
}
open_mode;

namespace detail
{
  class file;
}

class shiftable_file
{
  private:

    detail::file * m_pfile;

    int8_t * m_pmap;     // Address of mapped file in memory

    uint32_t m_abs_pos;  // Current pos. for read/write/insert...
    uint32_t m_cur_node; // Node and relative position in its
    uint32_t m_rel_pos;  // associated block (for faster access)

    // NOTE: The fields cur_node and rel_pos
    //       are multiprocess-unfriendly

    bool m_disable_shrink;    // true == don't ever shrink the file

    void init ();    // Initialize: no file, no map...

    uint32_t alloc_nodes (uint32_t num);    // Extract from free list

    uint32_t alloc_node ()                  // Extract from free list
    { return alloc_nodes (1); }

    void free_nodes_contiguous (uint32_t first,  // Add to free list
                                uint32_t num);

    void free_nodes_list (uint32_t first,        // Add to free list
                          uint32_t num);

    void free_node (uint32_t pos)                // Add to free list
    { free_nodes_contiguous (pos, 1); }

    void unfree_node (uint32_t pos);   // Remove node from free list

    static void expanded_size
                       (uint32_t data_size,         // Compute required
                        uint32_t & map_size,        // map size for a
                        uint32_t & meta_data_size,  // specific amount
                        bool min);                  // of data

    static void defragmented_layout               // Compute layout of
                       (uint32_t data_size,       // a file in the
                        uint32_t meta_data_size,  // less fragmented
                        uint32_t size[2],         // state possible
                        uint32_t pos[2]);

    uint32_t make_list_of_nodes (uint32_t size[2],  // Helper for open()
                                 uint32_t pos[2]);  // and defrag()

    bool worth_rebuild (uint32_t diff_nodes,// Decide whether rebuild
                        bool erase) const;  // the whole tree or not

    uint32_t build_tree (uint32_t first,    // Build a new tree with
                         uint32_t num);     // num nodes from a list

    void compact_data (bool fix_tree);      // Compact data
    void defrag (bool fix_tree);            //  " and sort blocks

    bool grow (uint32_t num_blocks);        // Helper methods
    bool shrink (uint32_t num_blocks=0);    // for resize(), write()...

    void move_node (uint32_t from,          // Move a node (and its
                    uint32_t to,            // data block if required)
                    bool block_too=true,
                    bool fix_tree=true);

    void swap_nodes (uint32_t from,         // Swap two nodes
                     uint32_t to);          // (block_too, !fix_tree)

    void update_counters (uint32_t n);                // Tree
    void update_counters_and_rebalance (uint32_t n);  // maintenance

    void find_pos (uint32_t pos,               // Find the position
                   uint32_t & cur_node,        // in the tree of a
                   uint32_t & rel_pos) const;  // given byte

    uint32_t make_room (uint32_t & cur_node,   // Make room for
                        uint32_t & rel_pos);   // inserting data

    uint32_t extract_node (uint32_t e);

    shiftable_file (const shiftable_file &)
    { init (); }

    const shiftable_file & operator= (const shiftable_file &)
    { return *this; }

  public:

    shiftable_file ()
    { init (); }

    ~shiftable_file ()
    { if (is_open()) close(false); }

    static uint32_t log2_block_size ()
    { return LOG2_BLOCK_SIZE; }

    bool is_open () const
    { return m_pfile!=NULL; }

    bool open (const char * name=NULL,
               open_mode mode=om_create_or_wipe_contents,
               file_format format=ff_auto_detect);

    void close (bool restore=true);

    void defrag ()
    { if (is_open()) { check_integrity (true); defrag (true); } }

    void compact_data ()
    { if (is_open()) { check_integrity (true); compact_data (true); } }

    uint32_t size () const;
    bool resize (uint32_t new_size);

    void stats (uint32_t & map_size,
                uint32_t & meta_data_size,
                uint32_t & used_count,
                uint32_t & free_count) const;

    uint32_t tell () const
    { return m_abs_pos; }

    void seek_set (uint32_t pos);

    void seek_cur (int pos)
    { seek_set (pos>=0||(uint32_t)-pos<m_abs_pos?m_abs_pos+pos:0); }

    void seek_end (int pos)
    { seek_set (pos>=0||(uint32_t)-pos<size()?size()+pos:0); }

    uint32_t read (void * buff, uint32_t bytes);
    uint32_t write (const void * buff, uint32_t bytes);
    uint32_t insert (const void * buff, uint32_t bytes);
    uint32_t remove (uint32_t bytes);

    bool disable_shrink (bool disable=true);

#if defined(_DEBUG) || defined(DEBUG)
    bool check_integrity (bool check_pos=false) const;
    bool check_subtree (uint32_t cur,
                        std::vector<bool> & flags,
                        uint32_t first,
                        uint32_t num) const;

#else
    bool check_integrity (bool check_pos=false) const
    { return true; }
#endif
};

} // namespace shiftable_files

#endif
