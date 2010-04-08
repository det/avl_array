///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2010, Universidad de Alcala                    //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
    Shiftable Files is a library that offers the typical file
    primitives (read/write and random access) plus fast insert/
    remove primitives.

    This library is strongly related to AVL Array in two ways. On
    one side, it is also based on augmented trees. On the other
    side, an AVL Array is the perfect complement for a Shiftable
    File when the programmer needs a tool for keeping track of
    the contents of the file as data are inserted/removed.

    AVL Array (which now includes Shiftable Files) is a Free
    Software Project hosted at:
    http://avl-array.sourceforge.net

    ---------------------------------------------------------------
    HOW TO USE:

    In order to compile the library, add to your makefile/project
    the next files:

        - A source file with a main() function (like test.cpp)
        - The main header file (shiftable_files.hpp)
        - The implementation of the library (detail/*)
        - The main OSAL files (osal/*)
        - _One_ of the OSAL implementations, depending on the
          platform (osal/posix/* _or_ osal/windows/* _or_
          osal/any/*)

    NOTE: The Windows OSAL requires the Windows SDK.

    If you need to port Shiftable Files to another Operating
    System, use osal/template/* as a guide.

    ---------------------------------------------------------------
    INITIAL RELEASE NOTES OF SHIFTABLE FILES (ALPHA RELEASE):

    The library is fully operational in Windows and Linux. It has
    been tested with thousands of operations on files of different
    sizes (up to several MB).

    The main pending subjects are:

      * Error control when mapping operations fail.
      * Error control when the maximum size is reached.
      * Error recovery (repair damaged files).
      * Support for large files (more than 4GB).

    Due to these isues, the current version must be labelled as
    alpha release.


                        April, 2010
                        Martin Knoblauch Revuelta
                        Universidad de Alcala (Spain)
                        comocomocomo at users.sourceforge.net
*/

