#include "xml_reader.hpp"
#include "osm_data.hpp"
#include "dbhandler.hpp"

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

template<typename T1, typename T2>
void requireEqual( const T1 &val1, const T2 &val2, const std::string &message )
{
    if ( val1 != val2 )
    {
        std::cerr << message << " (" << val1 << ", " << val2 << ")" << std::endl;
    }

}

void compareOSMXML( const OSMFragment &frag1, const OSMFragment &frag2 )
{
    requireEqual( frag1.getVersion(), frag2.getVersion(), "Versions do not match" );
    requireEqual( frag1.getGenerator(), frag2.getGenerator(), "Generators do not match" );
    requireEqual( frag1.getNodes().size(), frag2.getNodes().size(), "Number of nodes does not match" );
    requireEqual( frag1.getWays().size(), frag2.getWays().size(), "Number of ways does not match" );
    requireEqual( frag1.getRelations().size(), frag2.getRelations().size(), "Number of relations does not match" );
}

void readOSMXML( XercesInitWrapper &x, const std::string &fileName, OSMFragment &frag )
{
    std::cout << "Reading XML file: " << fileName << std::endl;

    xercesc::SAX2XMLReaderImpl &parser = x.getParser();

    boost::shared_ptr<XMLNodeData> startNdData( new XMLNodeData() );

    startNdData->registerMembers()( "osm", boost::bind( &OSMFragment::build, &frag, _1 ) );

    XMLReader handler( startNdData );

    parser.setContentHandler( &handler );
    parser.setErrorHandler( &handler );

    parser.parse( fileName.c_str() );

    std::cout << "Done..." << std::endl;
}


int main( int argc, char **argv )
{
    if ( argc != 3 )
    {
        std::cout << "Incorrect number of arguments" << std::endl;
        return -1;
    }

    std::string file1 = argv[1];
    std::string file2 = argv[2];

    try
    {
        XercesInitWrapper x;

        OSMFragment fragment1, fragment2;
         
        readOSMXML( x, file1, fragment1 );
        readOSMXML( x, file2, fragment2 );

        compareOSMXML( fragment1, fragment2 );
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
