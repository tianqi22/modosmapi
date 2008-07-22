
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
#include "quadtree.hpp"
#include "router.hpp"

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

typedef XYPoint<double> xyPoint_t;


class RouteApp
{
private:
    OSMFragment                     m_fullOSMData;
    boost::scoped_ptr<RoutingGraph> m_routingGraph;
    QuadTree<double, dbId_t>        m_nodeCoords;    

public:
    RouteApp( const std::string &mapFileName ) : m_nodeCoords( -90, 90, -180, 180, 12 )
    {
        std::cout << "Reading map data for file: " << mapFileName << std::endl;
        readMapData( mapFileName );
        buildRoutingGraph();
        std::cout << "Routeapp object construction complete" << std::endl;
    }

    void buildRoutingGraph()
    {
        std::cout << "Making routing graph object" << std::endl;
        m_routingGraph.reset( new RoutingGraph( m_fullOSMData ) );

        std::cout << "Building routing graph from OSM map data" << std::endl;
        boost::function<void( double, double, dbId_t )> fn( boost::bind(
            &QuadTree<double, dbId_t>::add,
            boost::ref( m_nodeCoords ),
            _1, _2, _3 ) );
        //m_routingGraph->build( fn );
    }

    void readMapData( const std::string &mapFileName )
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
};


int main( int argc, char **argv )
{
    if ( argc != 2 )
    {
        std::cerr << "Usage: routeapp <OSM XML file>" << std::endl;
        return -1;
    }

    RouteApp r( argv[1] );
}


