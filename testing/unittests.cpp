#include "xml_reader.hpp"
#include "osm_data.hpp"

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <boost/test/included/unit_test.hpp>


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


void xmlParseTestFn( std::string fileName )
{
    try
    {
        XercesInitWrapper x;
        
        testXMLRead( fileName, x );
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

    if ( argc != 2 )
    {
        std::cerr << "Incorrect usage: please supply input xml file name" << std::endl;
        exit( -1 );
    }
    
    std::string testFileName = argv[1];

    test->add( BOOST_TEST_CASE( boost::bind( &xmlParseTestFn, testFileName ) ) );

    return test;
}

