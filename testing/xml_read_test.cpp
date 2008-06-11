#include "xml_reader.hpp"
#include "osm_data.hpp"

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>


int main( int argc, char **argv )
{
    try
    {
        XercesInitWrapper x;

        xercesc::SAX2XMLReaderImpl &parser = x.getParser();

        XMLNodeData startNdData;

        OSMFragment newFragment;

        startNdData.registerMembers()( "osm", boost::bind( &OSMFragment::build, newFragment, _1 ) );

        XMLReader handler( startNdData );

        parser.setContentHandler( &handler );
        parser.setErrorHandler( &handler );

        parser.parse( "./testinput.osm" );

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
