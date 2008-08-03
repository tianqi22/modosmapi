
// Open a new (Q)MapControl window
// Pop up a couple of icons at pre chosen lon, lat to check it works

// Open and read an XML file into internal datastructure
// Build a QuadTree for all the nodes
// Display all the nodes in a given window on the MapControl
// Implement a click to find nearest

// Build a QuadTree for all the (routing) nodes and a routing graph
// Allow begin and end to be chosen and route between
// Implement one-way


// Read in OSM XML, building const string tag map
// Build routing map, including QuadTree
// Simple API


#include "xml_reader.hpp"
#include "osm_data.hpp"
#include "routeapp.hpp"

#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/asio.hpp>


RouteApp::RouteApp( const std::string &mapFileName ) : m_nodeCoords( 12, -90, 90, -180, 180 )
{
    std::cout << "Reading map data for file: " << mapFileName << std::endl;
    readMapData( mapFileName );
    std::cout << "Reading map data complete" << std::endl;
    buildRoutingGraph();
    std::cout << "Routeapp object construction complete" << std::endl;
}

void RouteApp::registerRouteNode( double x, double y, dbId_t nodeId, bool /*inRouteGraph*/ )
{
    m_nodeCoords.add( x, y, nodeId );
}


void RouteApp::buildRoutingGraph()
{
    std::cout << "Making routing graph object" << std::endl;
    m_routingGraph.reset( new RoutingGraph( m_fullOSMData ) );
    
    std::cout << "Building routing graph from OSM map data" << std::endl;
    boost::function<void( double, double, dbId_t, bool )> fn( boost::bind( &RouteApp::registerRouteNode, this, _1, _2, _3, _4 ) );
    m_routingGraph->build( fn );
}

void RouteApp::readMapData( const std::string &mapFileName )
{
    try
    {
        XercesInitWrapper x;
        
        readOSMXML( x, mapFileName, m_fullOSMData );
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


boost::shared_ptr<OSMNode> RouteApp::getClosestNode( xyPoint_t point )
{
    dbId_t idOfClosest = m_nodeCoords.closestPoint( point ).get<2>();
    
    return getNodeById( idOfClosest );
}

boost::shared_ptr<OSMNode> RouteApp::getNodeById( dbId_t nodeId )
{
    const OSMFragment::nodeMap_t &nodeMap= m_fullOSMData.getNodes();
    OSMFragment::nodeMap_t::const_iterator findIt = nodeMap.find( nodeId );
    
    if ( findIt == nodeMap.end() )
    {
        throw std::out_of_range( "Node not found in map" );
    }
    
    return findIt->second;
}

void RouteApp::calculateRoute( dbId_t sourceNodeId, dbId_t destNodeId, RoutingGraph::route_t &route )
{
    m_routingGraph->calculateRoute( sourceNodeId, destNodeId, route );
}

void parseString( const std::string &theString, std::map<std::string, std::vector<std::string> > &keyVals );


namespace asio=boost::asio;
using asio::ip::tcp;


class RouteSocketMon
{
    asio::io_service m_ioservice;
    RouteApp &m_routeApp;
    
public:
    RouteSocketMon( RouteApp &routeApp ) : m_routeApp( routeApp )
    {
    }

    void run();
    std::string parseRequest( const std::string &request );
};



void RouteSocketMon::run()
{
    tcp::acceptor acceptor( m_ioservice, tcp::endpoint( tcp::v4(), 5168 ) );

    tcp::socket socket( m_ioservice );
    acceptor.accept( socket );
    
    while ( true )
    {
        std::stringstream ss;
        while ( true )
        {
            boost::array<char, 128> buf;
            boost::system::error_code error;
            
            size_t len = socket.read_some( asio::buffer( buf ), error );
            
            ss.write( buf.data(), len );
            
            if ( buf[len-1] == '\n' ) break;
        }
        std::string request = ss.str();

        std::string result = parseRequest( request );

        // Now send the result back down the socket
        result += "\n";
        asio::write( socket, asio::buffer( result ), asio::transfer_all() );
    }
}

std::string RouteSocketMon::parseRequest( const std::string &request )
{
    std::map<std::string, std::vector<std::string> > keyVals;

    parseString( request, keyVals );

    std::string requestType = keyVals["request"][0];
    if ( requestType == "closest" )
    {
        // request=closest;coords=<lat>,<lon>
        std::vector<std::string> coords = keyVals["coords"];
        double lat = boost::lexical_cast<double>( coords[0] );
        double lon = boost::lexical_cast<double>( coords[1] );

        boost::shared_ptr<OSMNode> nearest = m_routeApp.getClosestNode( xyPoint_t( lat, lon ) );

        // closest=<nodeid>,<lat>,<lon>
        return boost::str( boost::format( "closest=%d,%f,%f" )
                           % nearest->getId()
                           % nearest->getLon()
                           % nearest->getLat() );

    }
    else if ( requestType == "route" )
    {
        // request=route;endpoints=<startnode>,<endnode>
        std::vector<std::string> endPoints = keyVals["endpoints"];

        dbId_t startId = boost::lexical_cast<dbId_t>( endPoints[0] );
        dbId_t endId   = boost::lexical_cast<dbId_t>( endPoints[1] );

        RoutingGraph::route_t route;
        m_routeApp.calculateRoute( startId, endId, route );

        // 0=<nodeid>,<lat>,<lon>;1=<nodeid>,<lat>,<lon>
        std::stringstream ss;
        size_t count = 0;
        std::vector<std::string> routeEls;
        BOOST_FOREACH( boost::shared_ptr<OSMNode> theNode, route )
        {
            dbId_t id = theNode->getId();
            double lat = theNode->getLat();
            double lon = theNode->getLon();

            routeEls.push_back( boost::str( boost::format( "%06d=%d,%f,%f" )
                                            % count++
                                            % id
                                            % lat
                                            % lon ) );
        }

        return boost::algorithm::join( routeEls, ";" );
    }
    else
    {
        return "Unrecognised request";
    }
}


int main( int argc, char **argv )
{
    RouteApp ra( argv[1] );

    RouteSocketMon sm( ra );

    sm.run();
}
