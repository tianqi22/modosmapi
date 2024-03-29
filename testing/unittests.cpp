#include "xml_reader.hpp"
#include "osm_data.hpp"
#include "dbhandler.hpp"
#include "quadtree.hpp"

//#include "engine.hpp"


#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/random.hpp>

#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

typedef boost::tuple<boost::uint64_t, std::string, boost::uint64_t, int, double, std::string, boost::posix_time::ptime> testTuple_t;

struct CallBackTester
{
    std::vector<testTuple_t> data;

    void addRow( const testTuple_t &row )
    {
        data.push_back( row );
    }
};

void testDbHandler()
{
    try
    {
        modosmapi::DbConnection db( "localhost", "openstreetmap", "openstreetmap", "openstreetmap" );

        db.executeNoResult( "CREATE TEMPORARY TABLE test_temp( id INT, value INT, PRIMARY KEY(id) )" );
        db.executeNoResult( "INSERT INTO test_temp VALUES ( 1, 2 ), (3, 4)" );

        db.execute( "SELECT * FROM test_temp ORDER BY id" );

        db.next();
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 1 );
        BOOST_CHECK_EQUAL( db.getField<int>( 1 ), 2 );


        boost::tuple<int, int> testTuple;
        db.readRow( testTuple );

        BOOST_CHECK_EQUAL( testTuple.get<0>(), 1 );
        BOOST_CHECK_EQUAL( testTuple.get<1>(), 2 );

        db.next();
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 3 );
        BOOST_CHECK_EQUAL( db.getField<int>( 1 ), 4 );

        db.executeNoResult( "DELETE FROM test_temp" );
        db.execute( "SELECT COUNT(*) from test_temp" );

        db.next();
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 0 );

        std::vector<boost::tuple<int, int> > bulkInsertVec;
        for ( int i = 0; i < 1000; i++ )
        {
            bulkInsertVec.push_back( boost::make_tuple( i, i * 3 ) );
        }


        db.executeBulkInsert( "INSERT INTO test_temp VALUES( ?, ? )", bulkInsertVec );

        db.execute( "SELECT COUNT(*) FROM test_temp" );
        db.next();
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 1000 );

        db.execute( "SELECT * FROM test_temp ORDER BY id" );
        for ( int i = 0; i < 1000; i++ )
        {
            db.next();
            BOOST_CHECK_EQUAL( db.getField<int>( 0 ), i );
            BOOST_CHECK_EQUAL( db.getField<int>( 1 ), i * 3 );
        }

        db.executeNoResult( "DROP TABLE test_temp" );
        db.executeNoResult( "CREATE TEMPORARY TABLE test_temp2( id INT, text1 VARCHAR(255), nid BIGINT, value1 INT, value2 DOUBLE, text2 TEXT, time DATETIME  )" );

        boost::posix_time::ptime now = boost::posix_time::ptime(
            boost::gregorian::date( 2008, 06, 10 ),
            boost::posix_time::time_duration( 18, 16, 00 ) );

        std::vector<testTuple_t> testVec2;
        for ( int i = 0; i < 100; i++ )
        {
            std::stringstream ss;
            ss << "Hello" << i;
            testVec2.push_back( boost::make_tuple( i, ss.str(), 1231283721 + i, i - 24, 3.14159254, "World", now ) );
        }

        db.executeBulkInsert( "INSERT INTO test_temp2 VALUES( ?, ?, ?, ?, ?, ?, ? )", testVec2 );

        db.execute( "SELECT COUNT(*) FROM test_temp2" );
        db.next();
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 100 );

        db.execute( "SELECT * from test_temp2 LIMIT 1" );
        db.next();
        BOOST_CHECK_EQUAL( db.getField<boost::uint64_t>( 0 ), 0 );
        BOOST_CHECK_EQUAL( db.getField<std::string>( 1 ), "Hello0" );
        BOOST_CHECK_EQUAL( db.getField<boost::uint64_t>( 2 ), 1231283721 );
        BOOST_CHECK_EQUAL( db.getField<int>( 3 ), -24 );
        BOOST_CHECK_CLOSE( db.getField<double>( 4 ), 3.141592654, 0.00001 );
        BOOST_CHECK_EQUAL( db.getField<std::string>( 5 ), "World" );
        //std::cout << db.getField<std::string>( 6 ) << std::endl;
        //BOOST_CHECK_EQUAL( db.getField<boost::posix_time::ptime>( 7 ), now );

         std::vector<testTuple_t> testVec3;
         db.executeBulkRetrieve( "SELECT id, text1, nid, value1, value2, text2, time FROM test_temp2 ORDER BY id", testVec3 );
         for ( int i = 0; i < 100; i++ )
         {
             const testTuple_t &thisTuple = testVec3[i];

             BOOST_CHECK_EQUAL( thisTuple.get<0>(), i );
             BOOST_CHECK_EQUAL( thisTuple.get<1>(), boost::str( boost::format( "Hello%d" ) % i ) );
             BOOST_CHECK_EQUAL( thisTuple.get<2>(), 1231283721 + i );
             BOOST_CHECK_EQUAL( thisTuple.get<3>(), i - 24 );
             BOOST_CHECK_CLOSE( thisTuple.get<4>(), 3.141592654, 0.00001 );
             BOOST_CHECK_EQUAL( thisTuple.get<5>(),  "World" );
             BOOST_CHECK_EQUAL( thisTuple.get<6>(), now );
         }

         CallBackTester t;
         boost::function<void( const testTuple_t & )> fn = boost::bind( &CallBackTester::addRow, boost::ref( t ), _1 );
         db.executeBulkRetrieve( "SELECT id, text1, nid, value1, value2, text2, time FROM test_temp2 ORDER BY id", fn );

         for ( int i = 0; i < 100; i++ )
         {
             const testTuple_t &thisTuple = t.data[i];

             BOOST_CHECK_EQUAL( thisTuple.get<0>(), i );
             BOOST_CHECK_EQUAL( thisTuple.get<1>(), boost::str( boost::format( "Hello%d" ) % i ) );
             BOOST_CHECK_EQUAL( thisTuple.get<2>(), 1231283721 + i );
             BOOST_CHECK_EQUAL( thisTuple.get<3>(), i - 24 );
             BOOST_CHECK_CLOSE( thisTuple.get<4>(), 3.141592654, 0.00001 );
             BOOST_CHECK_EQUAL( thisTuple.get<5>(),  "World" );
             BOOST_CHECK_EQUAL( thisTuple.get<6>(), now );
         }

         db.executeNoResult( "DROP TABLE test_temp2" );
    }
    catch( std::exception &e )
    {
        BOOST_FAIL( std::string( "Exception thrown with message: " ) + e.what() );
    }
}


void testXMLRead( std::string fileName, XercesInitWrapper &x )
{
    OSMFragment newFragment;
    readOSMXML( x, fileName.c_str(), newFragment );

    BOOST_CHECK_EQUAL( newFragment.getVersion(), "0.5" );
    BOOST_CHECK_EQUAL( newFragment.getGenerator(), "OpenStreetMap server" );
    BOOST_CHECK_EQUAL( newFragment.getNodes().size(), 5 );
    BOOST_CHECK_EQUAL( newFragment.getWays().size(), 1 );
    BOOST_CHECK_EQUAL( newFragment.getRelations().size(), 1 );

    const boost::shared_ptr<OSMWay> &theWay = newFragment.getWays().begin()->second;

    BOOST_CHECK_EQUAL( theWay->getId(), 3236218 );
    BOOST_CHECK_EQUAL( theWay->getVisible(), true );
    BOOST_CHECK_EQUAL( theWay->getTimeStamp(), boost::posix_time::ptime(
        boost::gregorian::date( 2008, 3, 2 ),
        boost::posix_time::time_duration( 22, 38, 37 ) ) );

    BOOST_CHECK_EQUAL( theWay->getUser(), "Antoine Sirinelli" );

    BOOST_CHECK_EQUAL( theWay->getTags().find("highway")->second, "tertiary" );
    BOOST_CHECK_EQUAL( theWay->getTags().find("name")->second, "Meadow Prospect" );
    BOOST_CHECK_EQUAL( theWay->getTags().find("created_by")->second, "Potlatch 0.7b" );

}


void xmlParseTestFn()
{
    try
    {
        XercesInitWrapper x;
        
        testXMLRead( "testing/testinput.xml", x );
    }
    catch ( const xercesc::XMLException &toCatch )
    {
        std::cerr << "Exception thrown in XML parse" << std::endl;
        throw;
    }
    catch ( const std::exception &e )
    {
        std::cerr << "Exception thrown in XML parse: " << e.what() << std::endl;
        throw;
    }
}

void tempMapQuery()
{
    //std::ofstream ofs( "temp.txt" );
    //modosmapi::Context context;
    //modosmapi::map( ofs, context, 51.7387, 51.7803, -1.3163, -0.4132 );
}

struct CountVisitor
{
    size_t m_count;
    CountVisitor() : m_count( 0 )
    {
    }
    
    void operator()( double x, double y, const std::string &value )
    {
        m_count++;
    }
};


void testOverlaps()
{
    typedef RectangularRegion<double> rect_t;

    rect_t A( 0.0, 0.0, 4.0, 4.0 );
    rect_t B( 2.0, 2.0, 6.0, 6.0 );
    rect_t C( 0.0, 2.0, 5.0, 4.0 );
    rect_t D( 2.0, 0.0, 3.0, 6.0 );
    rect_t E( 7.0, 0.0, 8.0, 6.0 );
    rect_t F( 0.0, 5.0, 1.0, 6.0 );
    rect_t G( 5.0, 0.0, 6.0, 5.0 );

    BOOST_ASSERT( A.overlaps( B ) );
    BOOST_ASSERT( B.overlaps( A ) );
    BOOST_ASSERT( B.overlaps( G ) );
    BOOST_ASSERT( G.overlaps( B ) );
    BOOST_ASSERT( !A.overlaps( F ) );
    BOOST_ASSERT( !F.overlaps( A ) );
    BOOST_ASSERT( !B.overlaps( F ) );
    BOOST_ASSERT( !F.overlaps( B ) );
    BOOST_ASSERT( !A.overlaps( G ) );
    BOOST_ASSERT( !G.overlaps( A ) );
    BOOST_ASSERT( C.overlaps( D ) );
    BOOST_ASSERT( D.overlaps( C ) );
    BOOST_ASSERT( !D.overlaps( E ) );
    BOOST_ASSERT( !E.overlaps( D ) );
    BOOST_ASSERT( !C.overlaps( E ) );
    BOOST_ASSERT( !E.overlaps( C ) );
    BOOST_ASSERT( A.overlaps( D ) );
    BOOST_ASSERT( D.overlaps( A ) );
    BOOST_ASSERT( C.overlaps( A ) );
    BOOST_ASSERT( A.overlaps( C ) );
    BOOST_ASSERT( C.overlaps( B ) );
    BOOST_ASSERT( B.overlaps( C ) );
    BOOST_ASSERT( !F.overlaps( C ) );
    BOOST_ASSERT( !C.overlaps( F ) );
    BOOST_ASSERT( !F.overlaps( D ) );
    BOOST_ASSERT( !D.overlaps( F ) );
    BOOST_ASSERT( !G.overlaps( D ) );
    BOOST_ASSERT( !D.overlaps( G ) );

    rect_t H( 4.0, -3.0, 9.0, 7.0 );
    rect_t I( 0.0, 5.0, 5.0, 10.0 );
    rect_t J( 5.0, -5.0, 10.0, 0 );

    BOOST_ASSERT( !I.overlaps( J ) );
    BOOST_ASSERT( H.overlaps( I ) );
    BOOST_ASSERT( H.overlaps( J ) );

    BOOST_ASSERT( !J.overlaps( I ) );
    BOOST_ASSERT( I.overlaps( H ) );
    BOOST_ASSERT( J.overlaps( H ) );
}

void testSplitStruct()
{
    typedef QuadTree<double, std::string>::SplitStruct splitStruct_t;
    splitStruct_t s( 5.0, 5.0, 10.0, 7.0, 0 );

    RectangularRegion<double> r0 = s.rectFor( splitStruct_t::QUAD_TL );
    RectangularRegion<double> r1 = s.rectFor( splitStruct_t::QUAD_TR );
    RectangularRegion<double> r2 = s.rectFor( splitStruct_t::QUAD_BL );
    RectangularRegion<double> r3 = s.rectFor( splitStruct_t::QUAD_BR );

    BOOST_CHECK_CLOSE( r0.m_minMin.m_x, -5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r0.m_minMin.m_y, -2.0, 1e-15 );
    BOOST_CHECK_CLOSE( r0.m_maxMax.m_x, 5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r0.m_maxMax.m_y, 5.0, 1e-15 );

    BOOST_CHECK_CLOSE( r1.m_minMin.m_x, 5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r1.m_minMin.m_y, -2.0, 1e-15 );
    BOOST_CHECK_CLOSE( r1.m_maxMax.m_x, 15.0, 1e-15 );
    BOOST_CHECK_CLOSE( r1.m_maxMax.m_y, 5.0, 1e-15 );

    BOOST_CHECK_CLOSE( r2.m_minMin.m_x, -5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r2.m_minMin.m_y, 5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r2.m_maxMax.m_x, 5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r2.m_maxMax.m_y, 12.0, 1e-15 );

    BOOST_CHECK_CLOSE( r3.m_minMin.m_x, 5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r3.m_minMin.m_y, 5.0, 1e-15 );
    BOOST_CHECK_CLOSE( r3.m_maxMax.m_x, 15.0, 1e-15 );
    BOOST_CHECK_CLOSE( r3.m_maxMax.m_y, 12.0, 1e-15 );
}

void testQuadTree()
{
    typedef QuadTree<double, std::string> qt_t;
    qt_t qt( 7, -10.0, 10.0, -10.0, 10.0 );
    boost::mt19937 rng;
    boost::uniform_real<double> u( -10.0, 10.0 );

    RectangularRegion<double> r( 4.0, -3.0, 9.0, 7.0 );

    std::vector<XYPoint<double> > points;

    size_t countInRegion = 0;
    for ( int i = 0; i < 10000; i++ )
    {
        XYPoint<double> point( u(rng), u(rng) );

        qt.add( point.m_x, point.m_y, boost::str( boost::format( "insertion %d" ) % i ) );

        if ( r.inRegion( point ) )
        {
            countInRegion++;
        }

        points.push_back( point );
    }

    CountVisitor cv;
    
    qt.visitRegion( r, boost::ref( cv ) );

    BOOST_CHECK_EQUAL( countInRegion, cv.m_count );

    for ( int i = 0; i < 100; i++ )
    {
        XYPoint<double> point( u(rng), u(rng) );

        // Find nearest by brute force
        double closestDist = std::numeric_limits<double>::infinity();
        XYPoint<double> nearest;
        BOOST_FOREACH( const XYPoint<double> &p, points )
        {
            double dist = distBetween( p.m_x, p.m_y, point.m_x, point.m_y );
            if ( dist < closestDist )
            {
                closestDist = dist;
                nearest = p;
            }
        }

        qt_t::coordEl_t qtPoint = qt.closestPoint( point );
        BOOST_CHECK_CLOSE( nearest.m_x, qtPoint.get<0>(), 1e-14 );
        BOOST_CHECK_CLOSE( nearest.m_y, qtPoint.get<1>(), 1e-14 );
    }
}


void testConstTagString()
{
    ConstTagString a( "One" );
    ConstTagString b( "Two" );
    ConstTagString c( "Three" );
    ConstTagString d( "Four" );
    ConstTagString e( "One" );
    ConstTagString f( "Two" );

    BOOST_CHECK_EQUAL( ConstTagString::numStrings(), 4 );

    BOOST_CHECK_EQUAL( a, e );
    BOOST_CHECK_EQUAL( b, f );

    BOOST_ASSERT( a != b );
    BOOST_ASSERT( a != c );
    BOOST_ASSERT( a != d );

    BOOST_CHECK_EQUAL( a, "One" );
    BOOST_CHECK_EQUAL( b, "Two" );

    BOOST_ASSERT( a != "Twelve" );

    BOOST_CHECK_EQUAL( ConstTagString::numStrings(), 5 );
}

boost::unit_test::test_suite* init_unit_test_suite( int argc, char **argv )
{
    boost::unit_test::test_suite *test = BOOST_TEST_SUITE( "Master test suite" );

    test->add( BOOST_TEST_CASE( &testDbHandler ) );
    test->add( BOOST_TEST_CASE( &testSplitStruct ) );
    test->add( BOOST_TEST_CASE( &testOverlaps ) );
    test->add( BOOST_TEST_CASE( &testQuadTree ) );
    test->add( BOOST_TEST_CASE( &testConstTagString ) );

    test->add( BOOST_TEST_CASE( &xmlParseTestFn ) );
    //test->add( BOOST_TEST_CASE( &tempMapQuery ) );
    return test;
}

