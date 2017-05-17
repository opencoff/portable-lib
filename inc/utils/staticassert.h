/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * static_assert.h - A compile time assert facility
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Sat Oct 29 17:36:12 2005
 *
 * Inspired by Alexandrescu's patterns.
 */

#ifndef __STATIC_ASSERT_H_1130625372_34130__
#define __STATIC_ASSERT_H_1130625372_34130__ 1

namespace putils
{


// template that allows us to construct a compile time checker.
// This is just a place holder
template<bool> struct static_checker
{
    static_checker(...) { }
};

// Overloaded definition - satisfy the cases when the assertion
// predicate is true. Note: the overload using 'false' is CORRECT!
template<> struct static_checker<false> { }


// Provide a compile time assert if 'x' is NOT true. The "message"
// 'm' must be a valid C++ identifier. It's helpful to make it long
// and descriptive. e.g.,
//      static_assert(is_derived_from(Foo, Bar), Foo__is__not__derived__from__Bar);
#define static_assert(x, m)  { \
            struct ERROR__##m { }; \
            (void)sizeof(static_checker<(x)!=0>(ERROR__##m));\
        }
    
}

#endif /* ! __STATIC_ASSERT_H_1130625372_34130__ */

/* EOF */
