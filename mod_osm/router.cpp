
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/astar_search.hpp>


#include <iostream>
#include <map>
#include <cmath>

#include "osm_data.hpp"
#include "exceptions.hpp"

using namespace std;

const double PI = acos( -1.0 );

// Should return the distance in km between the two points
// Ref: http://jan.ucc.nau.edu/~cvm/latlon_formula.html
double distBetween( double lat1, double lon1, double lat2, double lon2 )
{
    double a1 = (lat1 / 180.0) * PI;
    double b1 = (lon1 / 180.0) * PI;
    double a2 = (lat2 / 180.0) * PI;
    double b2 = (lon2 / 180.0) * PI;

    return 6378.0 * acos(cos(a1)*cos(b1)*cos(a2)*cos(b2) + cos(a1)*sin(b1)*cos(a2)*sin(b2) + sin(a1)*sin(a2));
}

typedef boost::adjacency_list<
    // Both edges and all vertices stored in vectors
    boost::vecS,
    boost::vecS,
    // Access to both input and output edges, in a directed graph
    boost::bidirectionalS,
    boost::property<boost::vertex_name_t, int>,
    boost::property<boost::edge_index_t, size_t> > GraphType;

typedef boost::graph_traits<GraphType>::vertex_descriptor VertexType;
typedef boost::graph_traits<GraphType>::edge_descriptor EdgeType;

typedef boost::property_map<GraphType, boost::edge_weight_t>::type EdgeWeightMapType;
typedef boost::property_map<GraphType, boost::vertex_name_t>::type NodeIndexMapType;

struct WeightMapLookup
{
    GraphType &m_g;
    std::vector<double> m_lengths;

public:
    typedef EdgeType key_type;
    typedef double   value_type;
    typedef double&  reference;
    typedef boost::lvalue_property_map_tag category;

    WeightMapLookup( GraphType &g, std::vector<double> lengths );
    double get( EdgeType e ) const;
};


typedef std::map<boost::uint64_t, VertexType> nodeIdToVertexMap_t;

class DistanceHeuristic : public boost::astar_heuristic<GraphType, double>
{
private:
    const OSMFragment&         m_frag;
    NodeIndexMapType           m_nodeIndexMap;
    boost::shared_ptr<OSMNode> m_dest;
    
public:
    typedef VertexType Vertex;

    DistanceHeuristic( const OSMFragment &frag, NodeIndexMapType nodeIndexMap, VertexType dest );
    boost::shared_ptr<OSMNode> getNode( Vertex v );
    double operator()( Vertex v );
};

// Class for A* visitor to throw once the destination is reached
struct FoundGoalException {};

class AStarVisitor : public boost::default_astar_visitor
{
private:
    VertexType m_dest;

public:
    AStarVisitor( VertexType dest );
    void examine_vertex( VertexType u, const GraphType &g );
};

class RoutingGraph
{
    /* Member data */
    /***************/
private:
    const OSMFragment&                      m_frag;
    nodeIdToVertexMap_t                     m_nodeIdToVertexMap;
    GraphType                               m_graph;
    std::vector<std::string>                m_routeWayKeys;

    size_t                                  m_nextEdgeId;
    std::vector<double>                     m_edgeLengths;
    std::vector<boost::shared_ptr<OSMWay> > m_edgeWays;

public:
    RoutingGraph( const OSMFragment &frag );
    VertexType getVertex( boost::uint64_t nodeId );
    EdgeType addEdge( VertexType source, VertexType dest, double length, boost::shared_ptr<OSMWay> way );
    bool validRoutingWay( const boost::shared_ptr<OSMWay> &way );
    void build();
    void calculateRoute( boost::uint64_t sourceNodeId, boost::uint64_t destNodeId, std::list<boost::uint64_t> &route );
};


WeightMapLookup::WeightMapLookup( GraphType &g, std::vector<double> lengths ) : m_g( g ), m_lengths( lengths )
{
}

double WeightMapLookup::get( EdgeType e ) const
{
    typedef boost::property_map<GraphType, boost::edge_index_t>::type EdgeIdMap_t;
    EdgeIdMap_t edgeIds = boost::get( boost::edge_index, m_g );
    
    size_t edgeIndex = edgeIds[e];
    
    return m_lengths[edgeIndex];
}

double get( const WeightMapLookup &m, EdgeType e )
{
    return m.get( e );
}

DistanceHeuristic::DistanceHeuristic(
    const OSMFragment &frag,
    NodeIndexMapType nodeIndexMap,
    VertexType dest ) :
    m_frag( frag ),
    m_nodeIndexMap( nodeIndexMap )
{
    m_dest = getNode( dest );
}

boost::shared_ptr<OSMNode> DistanceHeuristic::getNode( Vertex v )
{
    boost::uint64_t nodeId = m_nodeIndexMap[v];
    
    OSMFragment::nodeMap_t::const_iterator findIt = m_frag.getNodes().find( nodeId );
    if ( findIt == m_frag.getNodes().end() )
    {
        throw modosmapi::ModException( "Node not found in node map" );
    }
    
    return findIt->second;
}

double DistanceHeuristic::operator()( Vertex v )
{
    boost::shared_ptr<OSMNode> theNode = getNode( v );
    
    return distBetween( m_dest->getLat(), m_dest->getLon(), theNode->getLat(), theNode->getLon() );
}

AStarVisitor::AStarVisitor( VertexType dest ) :m_dest( dest )
{
}

void AStarVisitor::examine_vertex( VertexType u, const GraphType &g )
{
    if ( u == m_dest )
    {
        throw FoundGoalException();
    }
}


RoutingGraph::RoutingGraph( const OSMFragment &frag ) : m_frag( frag ), m_nextEdgeId( 0 )
{
    // TODO: We may want to fill these from a config file (not that likely to change though...)
    m_routeWayKeys.push_back( "highway" );
    m_routeWayKeys.push_back( "cycleway" );
    m_routeWayKeys.push_back( "tracktype" );
}

VertexType RoutingGraph::getVertex( boost::uint64_t nodeId )
{
    nodeIdToVertexMap_t::const_iterator findIt = m_nodeIdToVertexMap.find( nodeId );
        
    if ( findIt == m_nodeIdToVertexMap.end() )
    {
        VertexType v = boost::add_vertex( m_graph );
        m_nodeIdToVertexMap.insert( std::make_pair( nodeId, v ) );

        NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, m_graph );
        nodeIndexMap[v] = nodeId;

        return v;
    }
    else
    {
        return findIt->second;
    }
}


EdgeType RoutingGraph::addEdge( VertexType source, VertexType dest, double length, boost::shared_ptr<OSMWay> way )
{
    // Returns a pair: Edge descriptor, bool (true if edge added)
    EdgeType thisEdge;
    bool successfulInsert;
        
    boost::tie( thisEdge, successfulInsert ) = boost::add_edge( source, dest, m_nextEdgeId++, m_graph );

    m_edgeLengths.push_back( length );
    m_edgeWays.push_back( way );
        
    return thisEdge;
}
    
bool RoutingGraph::validRoutingWay( const boost::shared_ptr<OSMWay> &way )
{
    const tagMap_t &tags = way->getTags();

    BOOST_FOREACH( const std::string &routeWayKey, m_routeWayKeys )
    {
        if ( tags.find( routeWayKey ) != tags.end() )
        {
            return true;
        }
    }

    return false;
}

    
void RoutingGraph::build()
{
    // First pass: count the number of ways each node belongs to
    typedef std::map<boost::uint64_t, size_t> nodeCountInWays_t;
    nodeCountInWays_t nodeCountInWays;
    BOOST_FOREACH( const OSMFragment::wayMap_t::value_type &v, m_frag.getWays() )
    {
        const boost::shared_ptr<OSMWay> &way = v.second;
        if ( validRoutingWay( way ) )
        {
            BOOST_FOREACH( boost::uint64_t nodeId, way->getNodes() )
            {
                nodeCountInWays[nodeId]++;
            }

            // Add one to the start and end nodes as they must feature in the routing graph
            nodeCountInWays[way->getNodes().front()]++;
            nodeCountInWays[way->getNodes().back()]++;
        }
    }
        
    //EdgeWeightMapType edgeWeightMap = boost::get( boost::edge_weight, m_graph );
    // Make a routing graph edge for each relevant section of each way
    const OSMFragment::nodeMap_t &nodeMap = m_frag.getNodes();
    BOOST_FOREACH( const OSMFragment::wayMap_t::value_type &v, m_frag.getWays() )
    {
        const boost::shared_ptr<OSMWay> &way = v.second;
        if ( validRoutingWay( way ) )
        {
            VertexType lastRouteVertex = VertexType();
            boost::shared_ptr<OSMNode> lastNode;
            double cumulativeDistance= 0.0;
            BOOST_FOREACH( boost::uint64_t nodeId, way->getNodes() )
            {
                const OSMFragment::nodeMap_t::const_iterator nFindIt = nodeMap.find( nodeId );
                if ( nFindIt == nodeMap.end() )
                {
                    throw modosmapi::ModException( "Node not found in node map" );
                }
                boost::shared_ptr<OSMNode> thisNode = nFindIt->second;
                    
                if ( nodeCountInWays[nodeId] > 1 )
                {
                    // Make this vertex and add it to the map
                    VertexType thisVertex = getVertex( nodeId );
                        
                    if ( lastRouteVertex )
                    {
                        addEdge( lastRouteVertex, thisVertex, cumulativeDistance, way );
                    }
                        
                    lastRouteVertex = thisVertex;
                    cumulativeDistance = 0.0;
                }
                else
                {
                    cumulativeDistance += distBetween(
                        lastNode->getLat(),
                        lastNode->getLon(),
                        thisNode->getLat(),
                        thisNode->getLon() );
                }

                lastNode = thisNode;
            }
        }
    }
}

// Assumes both nodes exists in the routing graph
void RoutingGraph::calculateRoute( boost::uint64_t sourceNodeId, boost::uint64_t destNodeId, std::list<boost::uint64_t> &route )
{
    // Get the begin and end vertices
    nodeIdToVertexMap_t::const_iterator findIt;
        
    findIt = m_nodeIdToVertexMap.find( sourceNodeId );
    if ( findIt == m_nodeIdToVertexMap.end() )
    {
        throw modosmapi::ModException( "Source node id not vertex map" );
    }
    VertexType sourceVertex = findIt->second;

    findIt = m_nodeIdToVertexMap.find( destNodeId );
    if ( findIt == m_nodeIdToVertexMap.end() )
    {
        throw modosmapi::ModException( "Dest node id not vertex map" );
    }
    VertexType destVertex = findIt->second;

    NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, m_graph );
    std::vector<VertexType> p( num_vertices( m_graph ) );
    std::vector<double> d( num_vertices( m_graph ) );        

    WeightMapLookup wml( m_graph, m_edgeLengths );

    try
    {
        astar_search( m_graph, sourceVertex,
                      DistanceHeuristic( m_frag, nodeIndexMap, destVertex ),
                      boost::predecessor_map( &p[0] ).
                      distance_map( &d[0] ).
                      weight_map( wml ).
                      visitor( AStarVisitor( destVertex ) ) );
    }
    catch ( FoundGoalException &e )
    {
        // End of route - we've reached the destination
        for ( VertexType v = destVertex; ; v = p[v] )
        {
            NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, m_graph );
            boost::uint64_t nodeId = nodeIndexMap[v];
            route.push_front( nodeId );

            if ( p[v] == v )
            {
                break;
            }
        }
    }        
}

