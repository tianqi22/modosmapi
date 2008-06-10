#include "xml_reader.hpp"
#include "osm_data.hpp"

#include <iostream>
#include <boost/shared_ptr.hpp>


void buildOSMFragment( XMLNodeData &data )
{
    // TODO: Clearly this needs to go somewhere...
    boost::shared_ptr<OSMFragment> newFragment( new OSMFragment( data ) );
}

int main( int argc, char **argv )
{
    try
    {
        XercesInitWrapper x;

        xercesc::SAX2XMLReaderImpl &parser = x.getParser();

        XMLNodeData startNdData;

        startNdData.registerMembers()( "osm", &buildOSMFragment );

        XMLReader handler( startNdData );

        parser.setContentHandler( &handler );
        parser.setErrorHandler( &handler );
        
        parser.parse( "/home/alex/devel/osm/data/oxford-cotswolds-080514.osm" );

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
