
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
#include "routeapp.hpp"


RouteApp::RouteApp( const std::string &mapFileName ) : m_nodeCoords( 12, -90, 90, -180, 180 )
{
    std::cout << "Reading map data for file: " << mapFileName << std::endl;
    readMapData( mapFileName );
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


// int main( int argc, char **argv )
// {
//     if ( argc != 2 )
//     {
//         std::cerr << "Usage: routeapp <OSM XML file>" << std::endl;
//         return -1;
//     }

//     RouteApp r( argv[1] );
// }


