///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  header.hpp
  ----------
  
  Definition of shiftable_files' header.
*/

#ifndef _SHF_HEADER_HPP_
#define _SHF_HEADER_HPP_

#include "../shiftable_files.hpp"

#ifdef __GNUC__
#define GCC_ATTR_PACKED __attribute__((__packed__))
#else
#define GCC_ATTR_PACKED
#endif

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

#define MAGIC_BYTES { 's', 'h', 'f', 'U', 'A', 'H', 'e', 's' }
#define NUM_MAGIC_BYTES 8

#define VERSION_HIGH  0
#define VERSION_LOW   1

#define CLOSED_OK         0U
#define OPEN              (1U<<0)
#define OP_MASK           (7U<<1)
#define OP_NONE           (0U<<1)
#define OP_RESIZE_GROW    (1U<<1)
#define OP_INSERT_GROW    (2U<<1)
#define OP_WRITE_GROW     (3U<<1)
#define OP_NORMAL_SHRINK  (4U<<1)
#define OP_DELAYED_SHRINK (5U<<1)

namespace shiftable_files
{

namespace detail
{

typedef union
{
  uint32_t num;
  int8_t bytes[sizeof(uint32_t)];
}
endianness_t;


class header
{
  private:
                                     // Signature for open files
    int8_t m_magic[NUM_MAGIC_BYTES]; // (non-compacted)

    int8_t m_version_high;
    int8_t m_version_low;
    int8_t m_sizeof_unsigned;

#if (NUM_MAGIC_BYTES+3)&7
    int8_t padding[8-((NUM_MAGIC_BYTES+3)&7)];
#endif

    uint32_t m_endianness;         // Configuration data for
    uint32_t m_block_size;         // detection of compatibility
                                   // isues
  public:

    uint32_t m_map_size;           // (size in bytes, metadata included)
    uint32_t m_meta_data_size;     // (metadata size in bytes)
    uint32_t m_free_list_first;    // (index of fist free node)
    uint32_t m_free_list_last;     // (index of last free node)
    uint32_t m_free_count;         // (number of free nodes)

    uint32_t m_state_flags;
    uint32_t m_op_start_pos;
    uint32_t m_op_bytes_requested;
    uint32_t m_op_bytes_done;

    void init ()    // Initialize the header (new file)
    {
      int i;
      int8_t m[NUM_MAGIC_BYTES] = MAGIC_BYTES;
      endianness_t e;

      for (i=0; i<NUM_MAGIC_BYTES; i++)
        m_magic[i] = m[i];

      m_version_high = VERSION_HIGH;
      m_version_low = VERSION_LOW;
      m_sizeof_unsigned = sizeof(uint32_t);

      for (i=0; i<(int)sizeof(e.bytes); i++)
        e.bytes[i] = (int8_t)i;

      m_endianness = e.num;
      m_block_size = BLOCK_SIZE;
    }

    bool check_magic_bytes () const
    {
      int i;
      int8_t m[NUM_MAGIC_BYTES] = MAGIC_BYTES;

      for (i=0; i<NUM_MAGIC_BYTES; i++)
        if (m_magic[i] != m[i])
          return false;

      return true;
    }

    bool verify_compatibility () const  // Check compatibility
    {                                   // of an existing file
      int i;
      endianness_t e = {0};

      for (i=0; i<(int)sizeof(e.bytes); i++)
        e.bytes[i] = (int8_t)i;

      if (m_version_high != VERSION_HIGH ||
          m_version_low != VERSION_LOW ||
          m_sizeof_unsigned != sizeof(uint32_t))
        return false;

      if (m_endianness != e.num)
        return false;

      if (m_block_size != BLOCK_SIZE)
        return false;

      return true;
    }

    void set_current_op (uint32_t op,
                         uint32_t start_pos=0,
                         uint32_t bytes_requested=0,
                         uint32_t bytes_done=0)
    {
      m_state_flags = (m_state_flags & ~OP_MASK) |
                      (op & OP_MASK);

      m_op_start_pos = start_pos;
      m_op_bytes_requested = bytes_requested;
      m_op_bytes_done = bytes_done;
    }
}
GCC_ATTR_PACKED;

} // namespace detail

} // namespace shiftable_files

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#undef GCC_ATTR_PACKED

#endif
