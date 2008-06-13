#include "xml_reader.hpp"
#include "osm_data.hpp"

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <boost/test/unit_test.hpp>

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

#define BOOST_CHECK_EQUAL( a, b ) if ( (a) != (b) ) { \
        std::cerr << "Check equal failed: " << a << " != " << b << " (" << __FILE__ << ", " << __LINE__ << ")" << std::endl; \
    } else {}


void testXMLRead( XercesInitWrapper &x )
{
    OSMFragment newFragment;
    readOSMXML( x, "./testinput.xml", newFragment );

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
        
        testXMLRead( x );       
    }
    catch ( const xercesc::XMLException &toCatch )
    {
        std::cerr << "Exception thrown in XML parse" << std::endl;
        return -1;
    }
    catch ( const std::exception &e )
    {
        std::cerr << "Exception thrown in XML parse: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}


test_suite *init_unit_test_suite( int argc, char **argv )
{
    test_suite *test = BOOST_TEST_SUITE( "Master test suite" );

    test->add( BOOST_TEST_CASE( &xmlParseTestFn ) );

    return test;
}