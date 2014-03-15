
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/intrusive/rbtree_algorithms.hpp>
#include <cassert>

namespace tt {

    struct data {
        int id;
        char c;

        // rbtree node
        data * parent;
        data * left;
        data * right;
        char color;


        data( int id, char c )
            : id( id )
            , c( c )
        {}

        data()
            : id( 0 )
            , c( ' ' )
        {}
    };


    template< typename T >
        struct rbtree_node_traits {
            typedef T           node;
            typedef T *         node_ptr;
            typedef T const *   const_node_ptr;
            typedef char        color;

            static node_ptr get_parent( const_node_ptr n ) { return n->parent; }
            static void set_parent( node_ptr n, node_ptr parent ) { n->parent = parent; }

            static node_ptr get_left( const_node_ptr n )       {  return n->left; }
            static void set_left( node_ptr n, node_ptr left )    {  n->left = left; }
            static node_ptr get_right(const_node_ptr n)        {  return n->right; }
            static void set_right(node_ptr n, node_ptr right)  {  n->right = right; }
            static color get_color(const_node_ptr n)           {  return n->color; }
            static void set_color(node_ptr n, color c)         {  n->color = c; }
            static color black()                               {  return color('b'); }
            static color red()                                 {  return color('r'); }
        };


    struct node_ptr_compare_less {
        bool operator()( data const * d, int id ) {
            return d->id < id;
        }
        bool operator()( int id, data const * d ) {
            return id < d->id;
        }
    };


    struct node_ptr_compare_greater {
        bool operator()( data const * d, int id ) {
            return d->id > id;
        }
        bool operator()( int id, data const * d ) {
            return id > d->id;
        }
    };


    struct node_ptr_compare_equal {
        bool operator()( data const * d, int id, char c ) {
            return d->id == id and d->c == c;
        }
        bool operator()( data const * a, data const * b ) {
            return ( a->id == b->id and a->c == b->c );
        }
    };


    struct disposer {
        typedef boost::intrusive::rbtree_algorithms< rbtree_node_traits< data > > algo;

        void operator()( const algo::node_ptr & n ) {
            delete n;
        }
    };


    typedef std::function< void( data * ) > ApplyFunc;


    template< typename T >
        void iterate( T * header, ApplyFunc func ) {
            typedef boost::intrusive::rbtree_algorithms< rbtree_node_traits< T > > algo;

            for ( typename algo::node_ptr it = algo::begin_node( header )
                    , end = algo::end_node( header );
                    it != end;
                    it = algo::next_node( it ) )
            {
                func( it );
            }
        }


    template< typename T >
        void iterateReverse( T * header, ApplyFunc func ) {
            typedef boost::intrusive::rbtree_algorithms< rbtree_node_traits< T > > algo;

            typename algo::node_ptr it( rbtree_node_traits< T >::get_right( header ) );
            typename algo::node_ptr end( algo::end_node( header ) );

            for ( ; it != end; it = algo::prev_node( it ) ) {
                func( it );
            }
        }


    struct DumpData {
        void operator()( data * d ) {
            std::cerr << d->id << "," << d->c << std::endl;
        }
    };


    struct op {

        typedef boost::intrusive::rbtree_algorithms< rbtree_node_traits< data > > algo;

        template< typename Compare >
            static bool insert_or_update(
                    data * header
                    , Compare comp
                    , int id, char c )
            {
                typedef std::pair< typename algo::node_ptr, bool > result;

                typename algo::insert_commit_data insert_data;
                result res = algo::insert_unique_check(
                        header, id, comp, insert_data );

                if ( res.second ) {
                    algo::insert_unique_commit(
                            header
                            , new data( id, c )
                            , insert_data );
                } else {
                    data * d( res.first );
                    d->id = id;
                    d->c = c;
                }

                return res.second;
            }
    };
}



bool rbtree_test_ascending() {

    typedef boost::intrusive::rbtree_algorithms< tt::rbtree_node_traits< tt::data > > algo;

    tt::node_ptr_compare_less compLess;
    tt::node_ptr_compare_equal compEqual;
    tt::disposer disposer;

    tt::data header;
    algo::init_header( & header );
    BOOST_TEST( algo::is_header( & header ) );

    tt::op::insert_or_update( & header, compLess, 1, 'a' );
    tt::op::insert_or_update( & header, compLess, 2, 'b' );
    tt::op::insert_or_update( & header, compLess, 3, 'c' );

    // iterate

    algo::node_ptr it = algo::begin_node( & header );
    algo::node_ptr end = algo::end_node( & header );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 1, 'a' ) );
    it = algo::next_node( it ); 

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 2, 'b' ) );
    it = algo::next_node( it );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 3, 'c' ) );
    it = algo::next_node( it );

    BOOST_TEST( compEqual( it, end ) );


    // reverse

    it = tt::rbtree_node_traits< tt::data >::get_right( & header );
    end = algo::end_node( & header );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 3, 'c' ) );
    it = algo::prev_node( it );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 2, 'b' ) );
    it = algo::prev_node( it );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 1, 'a' ) );
    it = algo::prev_node( it );


    BOOST_TEST( compEqual( it, end ) );

    algo::clear_and_dispose( & header, disposer );

    return true;
}



bool rbtree_test_descending() {

    typedef boost::intrusive::rbtree_algorithms< tt::rbtree_node_traits< tt::data > > algo;

    tt::node_ptr_compare_greater compGreater;
    tt::node_ptr_compare_equal compEqual;
    tt::disposer disposer;

    tt::data header;
    algo::init_header( & header );
    BOOST_TEST( algo::is_header( & header ) );

    tt::op::insert_or_update( & header, compGreater, 1, 'a' );
    tt::op::insert_or_update( & header, compGreater, 2, 'b' );
    tt::op::insert_or_update( & header, compGreater, 3, 'c' );

    // iterate

    algo::node_ptr it = algo::begin_node( & header );
    algo::node_ptr end = algo::end_node( & header );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 3, 'c' ) );
    it = algo::next_node( it );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 2, 'b' ) );
    it = algo::next_node( it );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 1, 'a' ) );
    it = algo::next_node( it ); 


    BOOST_TEST( compEqual( it, end ) );


    // reverse

    it = tt::rbtree_node_traits< tt::data >::get_right( & header );
    end = algo::end_node( & header );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 1, 'a' ) );
    it = algo::prev_node( it );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 2, 'b' ) );
    it = algo::prev_node( it );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 3, 'c' ) );
    it = algo::prev_node( it );


    BOOST_TEST( compEqual( it, end ) );

    algo::clear_and_dispose( & header, disposer );

    return true;
}


bool rbtree_test_one() {

    typedef boost::intrusive::rbtree_algorithms< tt::rbtree_node_traits< tt::data > > algo;

    tt::node_ptr_compare_less compLess;
    tt::node_ptr_compare_equal compEqual;
    tt::disposer disposer;

    tt::data header;
    algo::init_header( & header );
    BOOST_TEST( algo::is_header( & header ) );

    tt::op::insert_or_update( & header, compLess, 1, 'a' );

    // iterate

    algo::node_ptr it = algo::begin_node( & header );
    algo::node_ptr end = algo::end_node( & header );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 1, 'a' ) );
    it = algo::next_node( it ); 

    BOOST_TEST( compEqual( it, end ) );


    // reverse

    it = tt::rbtree_node_traits< tt::data >::get_right( & header );
    end = algo::end_node( & header );

    BOOST_TEST( ! compEqual( it, end ) );
    BOOST_TEST( compEqual( it, 1, 'a' ) );
    it = algo::prev_node( it );

    BOOST_TEST( compEqual( it, end ) );

    algo::clear_and_dispose( & header, disposer );

    return true;
}


bool rbtree_test_empty() {

    typedef boost::intrusive::rbtree_algorithms< tt::rbtree_node_traits< tt::data > > algo;

    tt::node_ptr_compare_equal compEqual;
    tt::disposer disposer;

    tt::data header;
    algo::init_header( & header );
    BOOST_TEST( algo::is_header( & header ) );

    // iterate

    algo::node_ptr it = algo::begin_node( & header );
    algo::node_ptr end = algo::end_node( & header );

    BOOST_TEST( compEqual( it, end ) );

    // reverse

    it = tt::rbtree_node_traits< tt::data >::get_right( & header );
    end = algo::end_node( & header );

    BOOST_TEST( compEqual( it, end ) );

    algo::clear_and_dispose( & header, disposer );

    return true;
}


int main() {
    return ( rbtree_test_ascending()
            and rbtree_test_descending()
            and rbtree_test_one()
            and rbtree_test_empty() ? 0 : 1 );
}

#include <boost/intrusive/detail/config_end.hpp>
