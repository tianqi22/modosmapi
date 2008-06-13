#include "xml_reader.hpp"
#include "osm_data.hpp"
#include "dbhandler.hpp"

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>


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


int main( int argc, char **argv )
{
    if ( argc != 2 )
    {
        std::cout << "Incorrect number of arguments" << std::endl;
        return -1;
    }

    std::string fileName = argv[1];

    try
    {
        XercesInitWrapper x;

        OSMFragment fragment;
         
        readOSMXML( x, fileName, fragment );

        boost::posix_time::ptime now(
            boost::gregorian::date( 2008, 06, 13 ),
            boost::posix_time::time_duration( 17, 0, 0 ) );

        typedef boost::tuples::cons<
            std::string,              boost::tuples::cons<
            id_t,                     boost::tuples::cons<
            int,                      boost::tuples::cons<
            std::string,              boost::tuples::cons<
            boost::posix_time::ptime, boost::tuples::cons<
            int,                      boost::tuples::cons<
            std::string,              boost::tuples::cons<
            double,                   boost::tuples::cons<
            double,                   boost::tuples::cons<
            int,                      boost::tuples::cons<
            int,                      boost::tuples::cons<
            std::string,              boost::tuples::cons<
            std::string,              boost::tuples::null_type> > > > > > > > > > > > > userRow_t;

        std::vector<userRow_t> userRows;
        BOOST_FOREACH( const OSMFragment::userMap_t::value_type &v, fragment.getUsers() )
        {
            userRow_t newUser = userRow_t( boost::make_tuple(
                "blah@blah.com",
                v.first,
                1,
                "DKJHSDKJS",
                now,
                v.second,
                1,
                "A bloke or a girl",
                0.0,
                0.0, boost::make_tuple(
                0,
                0,
                "shdkasj",
                "" ) );

            std::cout << "User id: " << v.first << ", with name: " << v.second << std::endl;
        }
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
