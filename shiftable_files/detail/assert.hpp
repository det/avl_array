///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  assert.hpp
  ----------

  Customizable assertion macros, useful for debugging in different
  platforms.
*/

#ifndef _SHF_ASSERT_HPP_
#define _SHF_ASSERT_HPP_

#include <iostream>
#include "types.hpp"

#define ALWAYS_TRUE (*"Yes"!=*"No")
#define ALWAYS_FALSE (*"Yes"==*"No")
#define SENTENCE(X) do{X}while(ALWAYS_FALSE)

// Choose what to do when a check fails:
//#define STOP   SENTENCE ( while(ALWAYS_TRUE) sleep (1); )   // CPU-friendly loop
#define STOP   SENTENCE ( while(ALWAYS_TRUE) {} )           // Infinite loop
//#define STOP   SENTENCE ( if (ALWAYS_TRUE) return false; )  // Continue (notify)
//#define STOP   SENTENCE ( ++ *((char*)0); )                 // Provoke fatal crash

#define SHF_ASSERT(X,M) SENTENCE (               \
  if(!(X))                                       \
  {                                              \
    std::cerr << std::endl                       \
         << __LINE__ << ": " << #X << std::endl  \
         << "ERROR: " << M << std::endl;         \
    std::cerr.flush ();                          \
    STOP;                                        \
  }                                              \
)

#if defined(_DEBUG) || defined(DEBUG)
#define shf_assert(X) SHF_ASSERT(X,__FILE__)
#else
#define shf_assert(X)
#endif

#endif

