///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  types.hpp
  ---------
  
  Definition of standard types (like int8_t) for MSVC.
*/

#ifndef _SHF_TYPES_HPP_
#define _SHF_TYPES_HPP_

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

#endif
