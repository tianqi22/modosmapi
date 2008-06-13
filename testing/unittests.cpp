#include "xml_reader.hpp"
#include "osm_data.hpp"
#include "dbhandler.hpp"

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

void testDbHandler()
{
    try
    {
        modosmapi::DbConnection db( "localhost", "openstreetmap", "openstreetmap", "openstreetmap" );

        db.executeNoResult( "CREATE TEMPORARY TABLE test_temp( id INT, value INT, PRIMARY KEY(id) )" );
        db.executeNoResult( "INSERT INTO test_temp VALUES ( 1, 2 ), (3, 4)" );

        db.execute( "SELECT * FROM test_temp ORDER BY id" );
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
        
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 0 );

        std::vector<boost::tuple<int, int> > bulkInsertVec;
        for ( int i = 0; i < 1000; i++ )
        {
            bulkInsertVec.push_back( boost::make_tuple( i, i * 3 ) );
        }

        db.executeBulkInsert( "INSERT INTO test_temp VALUES( ?, ? )", bulkInsertVec );

        db.execute( "SELECT COUNT(*) FROM test_temp" );
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 1000 );

        db.execute( "SELECT * FROM test_temp ORDER BY id" );
        for ( int i = 0; i < 1000; i++ )
        {
            BOOST_CHECK_EQUAL( db.getField<int>( 0 ), i );
            BOOST_CHECK_EQUAL( db.getField<int>( 1 ), i * 3 );
            db.next();
        }

        db.executeNoResult( "DROP TABLE test_temp" );

        db.executeNoResult( "CREATE TEMPORARY TABLE test_temp2( text1 VARCHAR(255), id BIGINT, value1 INT, value2 DOUBLE, text2 TEXT, time DATETIME  )" );

        typedef boost::tuple<std::string, boost::uint64_t, int, double, std::string, boost::posix_time::ptime> testTuple_t;

        boost::posix_time::ptime now = boost::posix_time::ptime(
            boost::gregorian::date( 2008, 06, 10 ),
            boost::posix_time::time_duration( 18, 16, 00 ) );

        std::vector<testTuple_t> testVec2;
        testVec2.push_back( boost::make_tuple( "Hello", 1231283721, -24, 3.14159254, "World", now ) );
        db.executeBulkInsert( "INSERT INTO test_temp2 VALUES( ?, ?, ?, ?, ?, ? )", testVec2 );

        db.execute( "SELECT COUNT(*) FROM test_temp2" );
        BOOST_CHECK_EQUAL( db.getField<int>( 0 ), 1 );

        db.execute( "SELECT * from test_temp2" );
        BOOST_CHECK_EQUAL( db.getField<std::string>( 0 ), "Hello" );
        BOOST_CHECK_EQUAL( db.getField<boost::uint64_t>( 1 ), 1231283721 );
        BOOST_CHECK_EQUAL( db.getField<int>( 2 ), -24 );
        BOOST_CHECK_CLOSE( db.getField<double>( 3 ), 3.141592654, 0.00001 );
        BOOST_CHECK_EQUAL( db.getField<std::string>( 4 ), "World" );
        //BOOST_CHECK_EQUAL( db.getField<boost::posix_time::ptime>( 5 ), now );

        db.executeNoResult( "DROP TABLE test_temp2" );
    }
    catch( modosmapi::SqlException &e )
    {
        BOOST_FAIL( std::string( "Exception thrown with message: " ) + e.getMessage() );
    }
}


void readOSMXML( XercesInitWrapper &x, const std::string &fileName, OSMFragment &frag )
{
    xercesc::SAX2XMLReaderImpl &parser = x.getParser();

    boost::shared_ptr<XMLNodeData> startNdData( new XMLNodeData() );

    startNdData->registerMembers()( "osm", boost::bind( &OSMFragment::build, &frag, _1 ) );

    XMLReader handler( startNdData );

    parser.setContentHandler( &handler );
    parser.setErrorHandler( &handler );

    parser.parse( fileName.c_str() );
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
    //BOOST_CHECK_EQUAL( theWay->getVisible(), true );
    BOOST_CHECK_EQUAL( theWay->getTimeStamp(), "2008-03-02T22:38:37+00:00" );
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


boost::unit_test::test_suite* init_unit_test_suite( int argc, char **argv )
{
    boost::unit_test::test_suite *test = BOOST_TEST_SUITE( "Master test suite" );

    test->add( BOOST_TEST_CASE( &xmlParseTestFn ) );
    test->add( BOOST_TEST_CASE( &testDbHandler ) );

    return test;
}

