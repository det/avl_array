///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
  test.cpp
  --------
  
  Create a file, write something, and then delete part of it.
  Close the file recompacting it (default behaviour for close).
  
  The resulting file can be viewed with any text editor.
*/

#include "shiftable_files.hpp"

using namespace shiftable_files;

int main ()
{
  shiftable_file shf;

  shf.open ("test.txt", om_create_or_wipe_contents);
  shf.write ("This is not a simple file.", 26);
  shf.seek_set (7);
  shf.remove (4);
  shf.close ();

  return 0;
}

