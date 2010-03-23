///////////////////////////////////////////////////////////////////
//                                                               //
//  Copyright (c) 2006-2009, Universidad de Alcala               //
//                                                               //
//  See accompanying LICENSE.TXT                                 //
//                                                               //
///////////////////////////////////////////////////////////////////

/*
    AVL Array is a C++ STL-style container with fast random access
    _and_ fast insert/erase at any point (all are O(log N) worst
    case)

    Free Software Project hosted at:
    http://avl-array.sourceforge.net

    ---------------------------------------------------------------
    VERSION 1.0 NOTES:

    The STL container 'vector' is perfect for random access, but it
    is slow on insert/erase operations. On the other hand, the STL
    container 'list' is fast on insert/erase operations, but it is
    slow on random access, since indexing requires traversing the
    list.

    In most real world applications, the best choice is one of
    these two containers. In some cases, though, both random access
    and insert/erase are required. An algorithm that uses
    frequently both kinds of operation on a single container will
    have the algorithmic complexity of the worst operation. If
    'vector' is chosen, it will be slow; and if the choice is
    'list', it will be slow too.

    The STL-style container 'avl_array', defined here, is intended
    to fill the gap between 'vector' and 'list'. It provides a
    reasonably fast random access (logarithimc time) and a
    reasonably fast insert/erase (also logarithic time).

    Additionally, it can be traversed as fast as a list (constant
    time per advance), and it provides algorithms like swap,
    reverse, sort, stable sort, merge*, unique* and binary search*
    (*: data must be previously in order).

    Last, but not least, all iterators defined here (yes: reverse
    iterators too) remain valid, following the referenced element,
    until it is erased. This holds true even when the referenced
    element is moved along the container or when it is moved from
    one container to another container.


    I'm a newbie on implementing STL-like templates, so if you
    see anything wrong here, have any suggestion, or decide to
    use AVL Arrays in something, please drop me some lines and
    tell me!  ;-)

    I must thank my colleague and friend Manuel Peinado Gallego,
    my personal templates-guru, who explained to me what are
    functors, tags, allocators, and other great weird things.


                        October, 2006
                        Martin Knoblauch Revuelta
                        University of Alcala (Spain)
                        comocomocomo at users.sourceforge.net


    ---------------------------------------------------------------
    VERSION 1.1 NOTES:

    An experimental feature, called "Non-Proportional Sequence
    View" has been added. It can be used by specifying a numeric
    type in the third operand of the template avl_array<T,A,W>. The
    specified type/class must have operators =, +, -, +=, -=, ==,
    !=, >, <, <=, >= and a conversion from int. The conversion from
    int will be used for the values 0 and 1.

    The "Non-Proportional Sequence View" isn't at all related to
    the width of the tree as a data structure. It is a feature that
    supports random access (also with O(log n) complexity) with
    index values different from the usual correlative natural
    numbers.

    The point here is that these index values don't need to be
    proportional to their ordinal positions in the sequence. For
    every node in the sequence, its non-proportional position is
    defined as the sum of the 'widths' of all previous elements in
    the sequence. The 'width' of an element is a new field. It is 1
    by default, leading to a classic index 0,1,2... Changing the
    'width' of an element (which takes O(log n) time) automatically
    alters the position of all elements placed after it in the
    sequence.

    The position of an element in this alternative view of the
    sequence can be retrieved in O(log n) time. The element of a
    given position in the alternative view of the sequence can be
    reached in O(log n) time.

    The order in the alternative, non-proportional view of the
    sequence is, of course, the same as in the normal sequence.
    The only difference is that, in the alternative view of the
    sequence, the index of elements doesn't need to be proportional
    to their ordinal position.

    When the width parameter (W) of avl_array<T,A,W> is not
    specified, an empty class is used by default, being all widths
    equivalent to 0.

    Iterators and operator[] remain unchanged (working with the
    traditional sequence of natural numbers). The only way to
    use this new feature is by calling the npsv_ methods:

      npsv_width(): retrieve the total width of the array. O(1)

      npsv_width(): retrieve the width of a node. O(1)
      npsv_set_width(): modify the width of a node. O(log n)

      npsv_pos_of(): given a node, get its position. O(log n)
      npsv_at_pos(): find a node, given its position. O(log n)

    See an example and more explanations in the file
    npvsexample.cpp

    Note that these methods are provided as a minimal interface.
    They might not be supported by later versions. With this
    minimal interface, changing the width of all elements takes
    O(n log n) time. With another more complete interface, this
    would take just O(n) time.

    Furthermore, this feature will probably fork the project in
    three different containers:

      1) The simple avl_array. Useful as a simple generic sequence
         container.

      2) An avl_array with both views. Useful for storing the data
         of a text editor (a long line, or paragraph in every
         element, storing the number of broken lines of every
         paragraph for the non-proportinal sequence view), or the
         different lines of a document or web page (using the
         non-proportional sequence view for vertical pixels).

      3) An avl_array working for the user as a simple generic
         sequence container, but using the non-proportional
         sequence view intrenally for storing a variable number of
         elements in every tree node (thus, saving space), being
         every tree node a fixed size (circular?) buffer.

    Currently, 1) and 2) are supported together by using a default
    value for the third parameter of the avl_array template.


                        October, 2006
                        Martin Knoblauch Revuelta
                        Universidad de Alcala (Spain)
                        comocomocomo at users.sourceforge.net


    ---------------------------------------------------------------
    VERSION 1.2 NOTES:

    Source reorganization and documentation to meet higher
    standards:

      * HTML documentation
      * Method implementations taken out of class declarations
      * Source code reorganized in 28 different files!
      * Use of standard types and allocator types
      * Concept checks (enabled if compiled with Boost)
      * Exceptions
      * Configurable assertions

    New features:

      * move() methods (splice too, for interface compatibility)
      * NPSV with lazy sums update
      * Multi NPSV

    Performance improvements:

      * Iterators are tagged now as bidirectional iterators
      * stable_sort() disabled by default (it reqires O(N) extra
        memory)
      * Now, Sequence constructor is O(N) too
      * Improved complexity estimation in operations with
        complexity O(min{n*log2(average_N),final_N}):

          - insert (iterator, n, t)
          - insert (iterator, from, to)
          - erase (from, n)
          - erase (from, to)
          - move (from, n, dest)
          - move (from, to, dest)
          - splice (dest, cont)
          - splice (dest, cont, from, to)
          - resize (size, t)
          - resize (size)

    Thanks to Jason Hise and Rene Rivera, for their revisions of
    v1.1 in the Boost developers mailing list (Oct-Nov 2006).


                        November, 2006
                        Martin Knoblauch Revuelta
                        Universidad de Alcala (Spain)
                        comocomocomo at users.sourceforge.net


    ---------------------------------------------------------------
    VERSION 1.2.1 NOTES:

    Code adapted to compile in the new versions of gcc.


    ---------------------------------------------------------------
    VERSION 1.4 NOTES (including notes of unpublished v1.3):

    Now, NPSV only requires operators =, +=, ==, <, and a
    conversion from int (used only for 0 and 1). In addition, the
    NPSV sums are always computed according to the sequence order.
    This way, NPSV types with non-commutative sums can be used.

    The class empty_number has been removed. It looked like an
    ellegant solution to the problem of NPSV and stable_sort
    occupying space when not needed. Sadly, empty structures (or
    classes) occupy one byte, so the trick was not working.

    The current trick to save space is using arrays of size 1
    (when the corresponding feature is used) or size 0 (when not
    used). This requires two additional template parameters to
    indicate whether NPSV and/or stable_sort are used or not. The
    current list of parameters is:

        <
          T,
          A  = std::allocator<T>,
          bW = false,             //  <--- New
          W  = std::size_t,
          bP = false,             //  <--- New
          P  = std::size_t
        >

    Note that zero-size arrays are a gcc language extension. In any
    other compiler, including msvc and mingw, the arrays will
    always have size 1. Therefore, you might prefer to specify the
    type char for W and/or P whenever you don't use them. For
    example, instead of:
                                   // No NPSV, no stable_sort,
        avl_array <int> my_array;  // but 3 unwanted std::size_t
                                   // per element if the compiler
                                   // is not gcc
        avl_array <
                    int,
                    std::allocator<int>,  // Same as default
                    false,                // No NPSV
                    char,                 // Minimize wasted space
                    false,                // No stable sort
                    char                  // Minimize wasted space
                  >
                  my_array;   // Just 3 unwanted bytes per element


                        December, 2009
                        Martin Knoblauch Revuelta
                        Universidad de Alcala (Spain)
                        comocomocomo at users.sourceforge.net
*/

