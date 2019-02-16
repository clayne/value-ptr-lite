//
// value-ptr-lite, a value_ptr type for C++98 and later.
// For more information see https://github.com/martinmoene/value-ptr-lite
//
// Copyright 2017-2018 by Martin Moene
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// ToDo:
// template < class U > /* EXPLICIT */ optional( const optional<U>& other );
// template < class U > /* EXPLICIT */ optional( optional<U>&& other );

#include "value_ptr-main.t.hpp"

using namespace nonstd;

namespace {

// The following tracer code originates as Oracle from Optional by
// Andrzej Krzemienski, https://github.com/akrzemi1/Optional.

enum State
{
    /* 0 */ default_constructed,
    /* 1 */ value_copy_constructed,
    /* 2 */ value_move_constructed,
    /* 3 */ copy_constructed,
    /* 4 */ move_constructed,
    /* 5 */ move_assigned,
    /* 6 */ copy_assigned,
    /* 7 */ value_copy_assigned,
    /* 8 */ value_move_assigned,
    /* 9 */ moved_from,
    /*10 */ value_constructed,
};

struct V
{
    State state;
    int   value;

    V(       ) : state( default_constructed ), value( deflt() ) {}
    V( int v ) : state( value_constructed   ), value( v       ) {}

    bool operator==( V const & rhs ) const { return state == rhs.state && value == rhs.value; }
    bool operator==( int       val ) const { return value == val; }

    static int deflt() { return 42; }
};

struct S
{
    State state;
    V     value;

    S(             ) : state( default_constructed    ) {}
    S( V const & v ) : state( value_copy_constructed ), value( v ) {}
    S( S const & s ) : state( copy_constructed       ), value( s.value        ) {}

    S & operator=( V const & v ) { state = value_copy_assigned; value = v; return *this; }
    S & operator=( const S & s ) { state = copy_assigned      ; value = s.value; return *this; }

#if nsvp_CPP11_OR_GREATER
    S(             V && v ) : state(  value_move_constructed ), value(  std::move( v       ) ) { v.state = moved_from; }
    S(             S && s ) : state(  move_constructed       ), value(  std::move( s.value ) ) { s.state = moved_from; }

    S & operator=( V && v ) { state = value_move_assigned     ; value = std::move( v       ); v.state = moved_from; return *this; }
    S & operator=( S && s ) { state = move_assigned           ; value = std::move( s.value ); s.state = moved_from; return *this; }
#endif

    bool operator==( S const & rhs ) const { return state == rhs.state && value == rhs.value; }
};

inline std::ostream & operator<<( std::ostream & os, V const & v )
{
    using lest::to_string;
    return os << "[V:" << to_string( v.value ) << "]";
}

inline std::ostream & operator<<( std::ostream & os, S const & s )
{
    using lest::to_string;
    return os << "[S:" << to_string( s.value ) << "]";
}

struct NoDefaultCopyMove
{
    std::string text;
    NoDefaultCopyMove( std::string txt ) : text( txt ) {}

private:
    NoDefaultCopyMove();
    NoDefaultCopyMove( NoDefaultCopyMove const & );
    void operator=   ( NoDefaultCopyMove const & );
#if nsvp_CPP11_OR_GREATER
    NoDefaultCopyMove( NoDefaultCopyMove && ) = delete;
    void operator=   ( NoDefaultCopyMove && ) = delete;
#endif
};

#if nsvp_CPP11_OR_GREATER
struct InitList
{
    std::vector<int> vec;
    char c;
    S s;

    InitList( std::initializer_list<int> il, char k, S const & t )
    : vec( il ), c( k ), s( t ) {}

    InitList( std::initializer_list<int> il, char k, S && t )
    : vec( il ), c( k ), s( std::move( t ) ) {}
};
#endif

} // anonymous namespace

//
// test specification:
//

//
// value_ptr member operations:
//

// construction:

CASE( "value_ptr: Allows to default construct an empty value_ptr" )
{
    value_ptr<int> a;

    EXPECT( !a );
}

CASE( "value_ptr: Allows to explicitly construct a disengaged, empty value_ptr via nullptr" )
{
#if nsvp_HAVE_NULLPTR
    value_ptr<int> a( nullptr );

    EXPECT( !a );
#else
    EXPECT( !!"value_ptr: std::nullptr_t is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to default construct an empty value_ptr with a non-default-constructible" )
{
//  FAILS: NoDefaultCopyMove x;
    value_ptr<NoDefaultCopyMove> a;

    EXPECT( !a );
}

CASE( "value_ptr: Allows to copy-construct from empty value_ptr" )
{
    value_ptr<int> a;

    value_ptr<int> b( a );

    EXPECT( !b );
}

CASE( "value_ptr: Allows to copy-construct from non-empty value_ptr" )
{
    value_ptr<int> a( 7 );

    value_ptr<int> b( a );

    EXPECT(  b      );
    EXPECT( *b == 7 );
    EXPECT(  b.get() != a.get() );
}

CASE( "value_ptr: Allows to move-construct from value_ptr (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    value_ptr<int> b( value_ptr<int>( 7 ) );

    EXPECT( *b == 7 );
#else
    EXPECT( !!"value_ptr: move-construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to copy-construct from literal value" )
{
    value_ptr<int> a = 7;

    EXPECT(  a      );
    EXPECT( *a == 7 );
}

CASE( "value_ptr: Allows to copy-construct from value" )
{
    const int i = 7;
    value_ptr<int> a( i );

    EXPECT(  a      );
    EXPECT( *a == 7 );
}

CASE( "value_ptr: Allows to move-construct from value (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    value_ptr<S> a( std::move( s ) );

    EXPECT( a->value == 7                );
    EXPECT( a->state == move_constructed );
    EXPECT(  s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: move-construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to in-place construct from literal value (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    using pair_t = std::pair<char, int>;

    value_ptr<pair_t> a( in_place, 'a', 7 );

    EXPECT( a->first  == 'a' );
    EXPECT( a->second ==  7  );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to in-place copy-construct from value (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    char c = 'a'; S s( 7 );
    using pair_t = std::pair<char, S>;

    value_ptr<pair_t> a( in_place, c, s );

    EXPECT( a->first        == 'a' );
    EXPECT( a->second.value ==  7  );
    EXPECT( a->second.state == copy_constructed );
    EXPECT(         s.state != moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to in-place move-construct from value (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    char c = 'a'; S s( 7 );
    using pair_t = std::pair<char, S>;

    value_ptr<pair_t> a( in_place, c, std::move( s ) );

    EXPECT( a->first        == 'a' );
    EXPECT( a->second.value ==  7  );
    EXPECT( a->second.state == move_constructed );
    EXPECT(         s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to in-place copy-construct from initializer-list (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    value_ptr<InitList> a( in_place, { 7, 8, 9, }, 'a', s );

    EXPECT( a->vec[0]  ==  7 );
    EXPECT( a->vec[1]  ==  8 );
    EXPECT( a->vec[2]  ==  9 );
    EXPECT( a->c       == 'a');
    EXPECT( a->s.value ==  7 );
    EXPECT( a->s.state == copy_constructed );
    EXPECT(    s.state != moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to in-place move-construct from initializer-list (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    value_ptr<InitList> a( in_place, { 7, 8, 9, }, 'a', std::move( s ) );

    EXPECT( a->vec[0]  ==  7  );
    EXPECT( a->vec[1]  ==  8  );
    EXPECT( a->vec[2]  ==  9  );
    EXPECT( a->c       == 'a' );
    EXPECT( a->s.value ==  7  );
    EXPECT( a->s.state == move_constructed );
    EXPECT(    s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to construct from pointer to value" )
{
    value_ptr<int> a( new int(7) );

    EXPECT( *a == 7 );
}

// assignment:

CASE( "value_ptr: Allows to assign nullptr to disengage (C++11)" )
{
#if nsvp_HAVE_NULLPTR
    value_ptr<int>  a( 7 );

    a = nullptr;

    EXPECT( !a );
#else
    EXPECT( !!"value_ptr: std::nullptr_t is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to copy-assign from/to engaged and disengaged value_ptr-s" )
{
    SETUP( "" )
    {
        value_ptr<int> d1;
        value_ptr<int> d2;
        value_ptr<int> e1( 123 );
        value_ptr<int> e2( 987 );

#if nsvp_HAVE_NULLPTR
    SECTION( "a disengaged value_ptr assigned nullptr remains empty" )
    {
        d1 = nullptr;

        EXPECT( !d1 );
    }
#endif
    SECTION( "a disengaged value_ptr assigned an engaged value_ptr obtains its value" )
    {
        d1 = e1;

        EXPECT(  d1 );
        EXPECT( *d1 == 123 );
        EXPECT(  d1.get() != e1.get() );
    }
    SECTION( "an engaged value_ptr assigned an engaged value_ptr obtains its value" )
    {
        e1 = e2;

        EXPECT(  e1 );
        EXPECT( *e1 == 987 );
        EXPECT(  e1.get() != e2.get() );
    }
#if nsvp_HAVE_NULLPTR
    SECTION( "an engaged value_ptr assigned nullptr becomes empty" )
    {
        e1 = nullptr;

        EXPECT( !e1 );
    }
#endif
    SECTION( "a disengaged value_ptr assigned a disengaged value_ptr remains empty" )
    {
        d1 = d2;

        EXPECT( !d1 );
        EXPECT(  d1.get() == d2.get() );
    }}
}

CASE( "value_ptr: Allows to move-assign from/to engaged and disengaged value_ptr-s (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    SETUP( "" )
    {
        value_ptr<int> d1;
        value_ptr<int> d2;
        value_ptr<int> e1( 123 );
        value_ptr<int> e2( 987 );

    SECTION( "a disengaged value_ptr assigned an engaged value_ptr obtains its value" )
    {
        d1 = std::move( e1 );

        EXPECT(  d1 );
        EXPECT( *d1 == 123 );
//      EXPECT(  d1.get() != e1.get() );
    }
    SECTION( "an engaged value_ptr assigned an engaged value_ptr obtains its value" )
    {
        e1 = std::move( e2 );

        EXPECT(  e1 );
        EXPECT( *e1 == 987 );
//      EXPECT(  e1.get() != e2.get() );
    }
    SECTION( "a disengaged value_ptr assigned a disengaged value_ptr remains empty" )
    {
        d1 = std::move( d2 );

        EXPECT( !d1 );
//      EXPECT(  d1.get() == d2.get() );
    }}
#else
    EXPECT( !!"value_ptr: move-assignment is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to copy-assign from literal value" )
{
    value_ptr<int> a;

    a = 7;

    EXPECT( *a == 7 );
}

CASE( "value_ptr: Allows to copy-assign from value" )
{
    const int i = 7;
    value_ptr<int> a;

    a = i;

    EXPECT( *a == i );
}

CASE( "value_ptr: Allows to move-assign from value (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    value_ptr<S> a;

    a = std::move( s );

    EXPECT( a->value == 7 );
    EXPECT( a->state == move_constructed );
    EXPECT(  s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to copy-emplace content from arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    using pair_t = std::pair<char, S>;
    S s( 7 );
    value_ptr<pair_t> a;

    a.emplace( 'a', s );

    EXPECT( a->first        == 'a' );
    EXPECT( a->second.value ==  7  );
    EXPECT( a->second.state == move_constructed );
    EXPECT(         s.state != moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to move-emplace content from arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    using pair_t = std::pair<char, S>;
    S s( 7 );
    value_ptr<pair_t> a;

    a.emplace( 'a', std::move( s ) );

    EXPECT( a->first        == 'a' );
    EXPECT( a->second.value ==  7  );
    EXPECT( a->second.state == move_constructed );
    EXPECT(         s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to copy-emplace content from intializer-list and arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    value_ptr<InitList> a;

    a.emplace( { 7, 8, 9, }, 'a', s );

    EXPECT( a->vec[0]  ==  7  );
    EXPECT( a->vec[1]  ==  8  );
    EXPECT( a->vec[2]  ==  9  );
    EXPECT( a->c       == 'a' );
    EXPECT( a->s.value ==  7  );
    EXPECT( a->s.state == move_constructed );
    EXPECT(    s.state != moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "value_ptr: Allows to move-emplace content from intializer-list and arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    value_ptr<InitList> a;

    a.emplace( { 7, 8, 9, }, 'a', std::move( s ) );

    EXPECT( a->vec[0]  ==  7  );
    EXPECT( a->vec[1]  ==  8  );
    EXPECT( a->vec[2]  ==  9  );
    EXPECT( a->c       == 'a' );
    EXPECT( a->s.value ==  7               );
    EXPECT( a->s.state == move_constructed );
    EXPECT(    s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

namespace cloner_deleter {

typedef int Movable;

struct Spy
{
    static void reset()
    {
        constructions() = clones() = destructions() = 0;
    }

    static int & constructions() { static int count = 0; return count; }
    static int & clones()        { static int count = 0; return count; }
    static int & destructions()  { static int count = 0; return count; }

    static Movable * create ( Movable const & value   ) { ++constructions(); return new Movable( value ); }
    static Movable * clone  ( Movable const & value   ) { ++clones();        return new Movable( value ); }
    static void      destroy( Movable       * pointer ) { ++destructions();  delete pointer; }
};

struct Cloner
{
    Movable *operator()( Movable const & value ) const { return Spy::clone( value ); }
};

struct Deleter
{
    void operator()( Movable * pointer ) const { return Spy::destroy( pointer ); }
};

} // anonymous namespace

CASE( "value_ptr: Allows to construct and destroy via user-specified cloner and deleter" )
{
    using namespace cloner_deleter;

    typedef value_ptr<Movable, Cloner, Deleter> Value_ptr;

    SETUP("")
    {

    Value_ptr a( Movable(42) );

    Spy::reset();

    SECTION( "constructed from pointer" )
    {{
        Value_ptr b( Spy::create( Movable(42) ) );

        EXPECT( *b == Movable(42) );
        EXPECT(  1 == Spy::constructions() );
        EXPECT(  0 == Spy::destructions()  );
        EXPECT(  0 == Spy::clones()        );
    }
        EXPECT(  1 == Spy::destructions()  );
    }

    SECTION( "copy-constructed" )
    {{
        Value_ptr b( a );

        EXPECT( *b == *a );
        EXPECT(  1 == Spy::clones() );
        EXPECT(  0 == Spy::destructions() );
    }
        EXPECT(  1 == Spy::destructions() );
    }

#if nsvp_CPP11_OR_GREATER
    SECTION( "move-constructed" )
    {{
        Value_ptr b( std::move(a) );

        EXPECT( *b == Movable(42) );
        EXPECT(  0 == Spy::clones() );
        EXPECT(  0 == Spy::destructions() );
    }
        EXPECT(  1 == Spy::destructions() );
    }
#endif
    }
}

namespace cloner {

struct Cloner : vptr::detail::default_clone<int>
{
    Cloner() : data(-1) {}
    int data;
};
}

CASE( "value_ptr: Allows to construct via user-specified cloner with member data" )
{
    using namespace cloner;

    SETUP("")
    {
        Cloner c; c.data = 7;

    SECTION( "default constructed" )
    {
        value_ptr<int, Cloner> vp;

        EXPECT( vp.get_cloner().data == -1 );
    }

    SECTION( "constructed from cloner object" )
    {
        value_ptr<int, Cloner> vp( c );

        EXPECT( vp.get_cloner().data == 7 );
    }

    SECTION( "constructed from value and cloner object" )
    {
        value_ptr<int, Cloner> vp( 42, c );

        EXPECT( *vp == 42 );
        EXPECT(  vp.get_cloner().data == 7 );
    }}
}

// observers:

struct Integer { int x; Integer(int v) : x(v) {} };

CASE( "value_ptr: Allows to obtain pointer to value via operator->()" )
{
    SETUP( "" )
    {
        value_ptr<Integer> e( Integer( 42 ) );

    SECTION( "operator->() yields pointer to value (const)" )
    {
        EXPECT(  e->x == 42 );
    }

    SECTION( "operator->() yields pointer to value (non-const)" )
    {
        e->x = 7;
        EXPECT(  e->x == 7 );
    }}
}

CASE( "value_ptr: Allows to obtain value via operator*()" )
{
    SETUP( "" )
    {
        value_ptr<int> e( 42 );

    SECTION( "operator*() yields value (const)" )
    {
        EXPECT( *e == 42 );
    }

    SECTION( "operator*() yields value (non-const)" )
    {
        *e = 7;
        EXPECT( *e == 7 );
    }}
}

CASE( "value_ptr: Allows to obtain moved-value via operator*()" )
{
}

CASE( "value_ptr: Allows to obtain engaged state via operator bool()" )
{
    value_ptr<int> a;
    value_ptr<int> b( 7 );

    EXPECT_NOT( a );
    EXPECT(     b );
}

CASE( "value_ptr: Allows to obtain engaged state via has_value()" )
{
    value_ptr<int> a;
    value_ptr<int> b( 7 );

    EXPECT_NOT( a.has_value() );
    EXPECT(     b.has_value() );
}

CASE( "value_ptr: Allows to obtain value via value()" )
{
    SETUP( "" )
    {
        value_ptr<int> e( 42 );

    SECTION( "value() yields value (const)" )
    {
        EXPECT( e.value() == 42 );
    }

    SECTION( "value() yields value (non-const)" )
    {
        e.value() = 7;

        EXPECT( e.value() == 7 );
    }}
}

CASE( "value_ptr: Allows to obtain value or default via value_or()" )
{
    SETUP( "" )
    {
        value_ptr<int> d;
        value_ptr<int> e( 42 );

    SECTION( "value_or( 7 ) yields value for non-empty value_ptr" )
    {
        EXPECT( e.value_or( 7 ) == 42 );
    }

    SECTION( "value_or( 7 ) yields default for empty value_ptr" )
    {
        EXPECT( d.value_or( 7 ) == 7 );
    }}
}

CASE( "value_ptr: Allows to obtain moved-default via value_or() (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    value_ptr<S> d;

    EXPECT( s.state == value_move_constructed );
    EXPECT( d.value_or( std::move(s) ).value == S(7).value );
    EXPECT( s.state == moved_from );
#else
    EXPECT( !!"value_ptr: move-semantics are not available (no C++11)" );
#endif
}

CASE( "value_ptr: Throws bad_value_access at disengaged access" )
{
    value_ptr<int>       vp;
    value_ptr<int> const cvp;
    
    EXPECT_THROWS_AS(  vp.value(), bad_value_access );
    EXPECT_THROWS_AS( cvp.value(), bad_value_access );
}

// modifiers:

CASE( "value_ptr: Allows to release its content" )
{
    value_ptr<int> a = 7;

    int * ap = a.release();

    EXPECT_NOT(  a       );
    EXPECT(     *ap == 7 ); delete ap;
}

CASE( "value_ptr: Allows to clear its content (reset)" )
{
    value_ptr<int> a = 7;

    a.reset();

    EXPECT_NOT( a );
}

CASE( "value_ptr: Allows to replace its content (reset)" )
{
    value_ptr<int> a;
    int * ap = new int( 7 );

    a.reset( ap );

    EXPECT(  a      );
    EXPECT( *a == 7 );
}

// swap:

CASE( "value_ptr: Allows to swap with other value_ptr (member)" )
{
    SETUP( "" )
    {
        value_ptr<int> d1;
        value_ptr<int> d2;
        value_ptr<int> e1( 42 );
        value_ptr<int> e2( 7 );

    SECTION( "swap disengaged with disengaged value_ptr" )
    {
        d1.swap( d2 );

        EXPECT( !d1 );
        EXPECT(  d1.get() == d2.get() );
    }

    SECTION( "swap engaged with engaged value_ptr" )
    {
        e1.swap( e2 );

        EXPECT(  e1  );
        EXPECT(  e2 );
        EXPECT( *e1 == 7 );
        EXPECT( *e2 == 42 );
        EXPECT(  e1.get() != e2.get() );
    }

    SECTION( "swap disengaged with engaged value_ptr" )
    {
        d1.swap( e1 );

        EXPECT(  d1 );
        EXPECT( !e1 );
        EXPECT( *d1 == 42 );
        EXPECT(  d1.get() != e1.get() );
    }

    SECTION( "swap engaged with disengaged value_ptr" )
    {
        e1.swap( d1 );

        EXPECT(  d1 );
        EXPECT( !e1 );
        EXPECT( *d1 == 42 );
        EXPECT(  d1.get() != e1.get() );
    }}
}

//
// value_ptr non-member functions:
//

CASE( "value_ptr: Allows to swap with other value_ptr (non-member)" )
{
    SETUP( "" )
    {
        value_ptr<int> d1;
        value_ptr<int> d2;
        value_ptr<int> e1( 42 );
        value_ptr<int> e2( 7 );

    SECTION( "swap disengaged with disengaged value_ptr" )
    {
        swap( d1, d2 );

        EXPECT( !d1 );
        EXPECT(  d1.get() == d2.get() );
    }
    SECTION( "swap engaged with engaged value_ptr" )
    {
        swap( e1, e2 );

        EXPECT(  e1  );
        EXPECT(  e2 );
        EXPECT( *e1 == 7 );
        EXPECT( *e2 == 42 );
        EXPECT(  e2.get() != e1.get() );
    }
    SECTION( "swap disengaged with engaged value_ptr" )
    {
        swap( d1, e1 );

        EXPECT(  d1 );
        EXPECT( !e1 );
        EXPECT( *d1 == 42 );
        EXPECT(  d1.get() != e1.get() );
    }
    SECTION( "swap engaged with disengaged value_ptr" )
    {
        swap( e1, d1 );

        EXPECT(  d1 );
        EXPECT( !e1 );
        EXPECT( *d1 == 42 );
        EXPECT(  d1.get() != e1.get() );
    }}
}

namespace compare_pointers {

    struct C
    {
        int * operator()( int const & x ) const
        {
            static int a[3];
            return & ( a[x] = -x );
        }
    };
    struct D
    {
        void operator()( int * ptr ) const {}
    };
}

CASE( "value_ptr: Provides relational operators (non-member, pointer comparison: nsvp_CONFIG_COMPARE_POINTERS!=0)" )
{
#if nsvp_CONFIG_COMPARE_POINTERS
    using namespace compare_pointers;

    SETUP( "" ) {

    value_ptr<int, C, D> e1( 1 );
    value_ptr<int, C, D> e2( 2 );

    SECTION( "engaged == engaged" ) { EXPECT(     e1 == e1  ); }
    SECTION( "engaged != engaged" ) { EXPECT(     e1 != e2  ); }

    SECTION( "engaged <  engaged" ) { EXPECT(     e1 <  e2  ); }
    SECTION( "engaged <= engaged" ) { EXPECT(     e1 <= e1  ); }
    SECTION( "engaged <= engaged" ) { EXPECT(     e1 <= e2  ); }

    SECTION( "engaged >  engaged" ) { EXPECT(     e2 >  e1  ); }
    SECTION( "engaged >= engaged" ) { EXPECT(     e1 >= e1  ); }
    SECTION( "engaged >= engaged" ) { EXPECT(     e2 >= e1  ); }

#if nsvp_HAVE_NULLPTR
    auto np = nullptr;

    SECTION( "engaged == nullptr" ) { EXPECT_NOT( e1 == np ); }
    SECTION( "engaged != nullptr" ) { EXPECT(     e1 != np ); }

    SECTION( "engaged <  nullptr" ) { EXPECT_NOT( e1 <  np ); }
    SECTION( "engaged <= nullptr" ) { EXPECT_NOT( e1 <= np ); }

    SECTION( "engaged >  nullptr" ) { EXPECT(     e1 >  np ); }
    SECTION( "engaged >= nullptr" ) { EXPECT(     e1 >= np ); }

    SECTION( "nullptr == engaged" ) { EXPECT_NOT( np == e1 ); }
    SECTION( "nullptr != engaged" ) { EXPECT(     np != e1 ); }

    SECTION( "nullptr <  engaged" ) { EXPECT(     np <  e1 ); }
    SECTION( "nullptr <= engaged" ) { EXPECT(     np <= e1 ); }

    SECTION( "nullptr >  engaged" ) { EXPECT_NOT( np >  e1 ); }
    SECTION( "nullptr >= engaged" ) { EXPECT_NOT( np >= e1 ); }
#endif
    }
#else
    EXPECT( !!"value_ptr: pointer comparison is not available (nsvp_CONFIG_COMPARE_POINTERS undefined or 0)" );
#endif
}

CASE( "value_ptr: Provides relational operators (non-member, value comparison: nsvp_CONFIG_COMPARE_POINTERS==0)" )
{
#if ! nsvp_CONFIG_COMPARE_POINTERS
    using namespace compare_pointers;

    SETUP( "" ) {

    value_ptr<int> de;
    value_ptr<int> e1( 1 );
    value_ptr<int> e2( 2 );

    SECTION( "engaged    == engaged"    ) { EXPECT(     e1 == e1  ); }
    SECTION( "engaged    != engaged"    ) { EXPECT(     e1 != e2  ); }

    SECTION( "engaged    <  engaged"    ) { EXPECT(     e1 <  e2  ); }
    SECTION( "engaged    <= engaged"    ) { EXPECT(     e1 <= e1  ); }
    SECTION( "engaged    <= engaged"    ) { EXPECT(     e1 <= e2  ); }

    SECTION( "engaged    >  engaged"    ) { EXPECT(     e2 >  e1  ); }
    SECTION( "engaged    >= engaged"    ) { EXPECT(     e1 >= e1  ); }
    SECTION( "engaged    >= engaged"    ) { EXPECT(     e2 >= e1  ); }

    SECTION( "disengaged == disengaged" ) { EXPECT(     de == de  ); }
    SECTION( "disengaged != disengaged" ) { EXPECT_NOT( de != de  ); }
    SECTION( "disengaged <  disengaged" ) { EXPECT_NOT( de <  de  ); }
    SECTION( "disengaged <= disengaged" ) { EXPECT(     de <= de  ); }
    SECTION( "disengaged >  disengaged" ) { EXPECT_NOT( de >  de  ); }
    SECTION( "disengaged >= disengaged" ) { EXPECT(     de >= de  ); }

    SECTION( "engaged    == disengaged" ) { EXPECT_NOT( e1 == de  ); }
    SECTION( "engaged    != disengaged" ) { EXPECT(     e1 != de  ); }
    SECTION( "engaged    <  disengaged" ) { EXPECT_NOT( e1 <  de  ); }
    SECTION( "engaged    <= disengaged" ) { EXPECT_NOT( e1 <= de  ); }
    SECTION( "engaged    >  disengaged" ) { EXPECT(     e1 >  de  ); }
    SECTION( "engaged    >= disengaged" ) { EXPECT(     e1 >= de  ); }

    SECTION( "disengaged == engaged"    ) { EXPECT_NOT( de == e1  ); }
    SECTION( "disengaged != engaged"    ) { EXPECT(     de != e1  ); }
    SECTION( "disengaged <  engaged"    ) { EXPECT(     de <  e1  ); }
    SECTION( "disengaged <= engaged"    ) { EXPECT(     de <= e1  ); }
    SECTION( "disengaged >  engaged"    ) { EXPECT_NOT( de >  e1  ); }
    SECTION( "disengaged >= engaged"    ) { EXPECT_NOT( de >= e1  ); }
    }
#else
    EXPECT( !!"value_ptr: value comparison is not available (nsvp_CONFIG_COMPARE_POINTERS is non-zero)" );
#endif
}

CASE( "value_ptr: Provides relational operators (non-member, mixed value comparison: nsvp_CONFIG_COMPARE_POINTERS==0)" )
{
#if ! nsvp_CONFIG_COMPARE_POINTERS
    using namespace compare_pointers;

    SETUP( "" ) {

    value_ptr<int> e1( 1 );
    value_ptr<int> e2( 2 );
    int            v1( 1 );
    int            v2( 2 );

    SECTION( "engaged == value"   ) { EXPECT(   e1 == v1  ); }
    SECTION( "engaged != value"   ) { EXPECT(   e1 != v2  ); }
    SECTION( "engaged <  value"   ) { EXPECT(   e1 <  v2  ); }
    SECTION( "engaged <= value"   ) { EXPECT(   e1 <= v1  ); }
    SECTION( "engaged <= value"   ) { EXPECT(   e1 <= v2  ); }
    SECTION( "engaged >  value"   ) { EXPECT(   e2 >  v1  ); }
    SECTION( "engaged >= value"   ) { EXPECT(   e1 >= v1  ); }
    SECTION( "engaged >= value"   ) { EXPECT(   e2 >= v1  ); }

    SECTION( "value   == engaged" ) { EXPECT(   v1 == e1  ); }
    SECTION( "value   != engaged" ) { EXPECT(   v2 != e1  ); }
    SECTION( "value   <  engaged" ) { EXPECT(   v1 <  e2  ); }
    SECTION( "value   <= engaged" ) { EXPECT(   v1 <= e1  ); }
    SECTION( "value   <= engaged" ) { EXPECT(   v1 <= e2  ); }
    SECTION( "value   >  engaged" ) { EXPECT(   v2 >  e1  ); }
    SECTION( "value   >= engaged" ) { EXPECT(   v1 >= e1  ); }
    SECTION( "value   >= engaged" ) { EXPECT(   v2 >= e1  ); }
    }
#else
    EXPECT( !!"value_ptr: value comparison is not available (nsvp_CONFIG_COMPARE_POINTERS is non-zero)" );
#endif
}

CASE( "make_value: Allows to copy-construct value_ptr" )
{
    S s( 7 );

    EXPECT( make_value( s )->value == 7          );
    EXPECT(                s.state != moved_from );
}

CASE( "make_value: Allows to move-construct value_ptr (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );

    EXPECT( make_value( std::move( s ) )->value == 7          );
    EXPECT(                             s.state == moved_from );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "make_value: Allows to in-place copy-construct value_ptr from arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    using pair_t = std::pair<char, S>;

    S s( 7);
    auto a = make_value<pair_t>( 'a', s );

    EXPECT( a->first        == 'a' );
    EXPECT( a->second.value ==  7  );
    EXPECT( a->second.state == copy_constructed );
    EXPECT(         s.state != moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "make_value: Allows to in-place move-construct value_ptr from arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    using pair_t = std::pair<char, S>;

    S s( 7 );
    auto a = make_value<pair_t>( 'a', std::move( s ) );

    EXPECT( a->first        == 'a' );
    EXPECT( a->second.value ==  7  );
    EXPECT( a->second.state == move_constructed );
    EXPECT(         s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "make_value: Allows to in-place copy-construct value_ptr from initializer-list and arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    auto a = make_value<InitList>( { 7, 8, 9, }, 'a', s );

    EXPECT( a->vec[0]  ==  7  );
    EXPECT( a->vec[1]  ==  8  );
    EXPECT( a->vec[2]  ==  9  );
    EXPECT( a->c       == 'a' );
    EXPECT( a->s.value ==  7  );
    EXPECT( a->s.state == copy_constructed );
    EXPECT(    s.state != moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "make_value: Allows to in-place move-construct value_ptr from initializer-list and arguments (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    S s( 7 );
    auto a = make_value<InitList>( { 7, 8, 9, }, 'a', std::move( s ) );

    EXPECT( a->vec[0]  ==  7  );
    EXPECT( a->vec[1]  ==  8  );
    EXPECT( a->vec[2]  ==  9  );
    EXPECT( a->c       == 'a' );
    EXPECT( a->s.value ==  7  );
    EXPECT( a->s.state == move_constructed );
    EXPECT(    s.state == moved_from       );
#else
    EXPECT( !!"value_ptr: in-place construction is not available (no C++11)" );
#endif
}

CASE( "std::hash<>: Allows to obtain hash (C++11)" )
{
#if nsvp_CPP11_OR_GREATER
    value_ptr<int> a( 7 );
    value_ptr<int> b( 7 );

    EXPECT( std::hash<value_ptr<int> >()( a ) == std::hash<value_ptr<int> >()( a ) );
    EXPECT( std::hash<value_ptr<int> >()( b ) == std::hash<value_ptr<int> >()( b ) );
    EXPECT( std::hash<value_ptr<int> >()( a ) != std::hash<value_ptr<int> >()( b ) );
#else
    EXPECT( !!"std::hash<>: std::hash<> is not available (no C++11)" );
#endif
}

//------------------------------------------------------------------------
// Applets:

#include <iostream>

CASE( "value_ptr: xxx" "[.applet]" )
{
}

// end of file
