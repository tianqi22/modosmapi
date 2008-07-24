
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/astar_search.hpp>


#include <iostream>
#include <map>
#include <cmath>
#include <iomanip>

#include "utils.hpp"
#include "osm_data.hpp"
#include "exceptions.hpp"

#include "router.hpp"

using namespace std;

struct WeightMapLookup
{
    GraphType&                                     m_g;
    const std::vector<boost::shared_ptr<OSMWay> >& m_edgeWays;
    const std::vector<double>&                     m_lengths;
    wayWeightings_t                                m_wayWeightings;

public:
    typedef EdgeType key_type;
    typedef double   value_type;
    typedef double&  reference;
    typedef boost::lvalue_property_map_tag category;

    WeightMapLookup(
        GraphType &g,
        const std::vector<boost::shared_ptr<OSMWay> > &edgeWays,
        const std::vector<double>& lengths,
        wayWeightings_t wayWeightings );
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


WeightMapLookup::WeightMapLookup(
    GraphType &g,
    const std::vector<boost::shared_ptr<OSMWay> > &edgeWays,
    const std::vector<double>& lengths,
    wayWeightings_t wayWeightings ) :
    m_g( g ), m_edgeWays( edgeWays ), m_lengths( lengths ), m_wayWeightings( wayWeightings )
{
}


// TODO: This is really very inefficient. Probably want to pre-calculate a set of weights,
//       perhaps on request in a queue
double calculateWayWeight( boost::shared_ptr<OSMWay> theWay, const wayWeightings_t &wayWeightings )
{
    const tagMap_t &wayTags = theWay->getTags();
    BOOST_FOREACH( const wayWeighting_t &weightTuple, wayWeightings )
    {
        std::string key, value;
        double weight;

        boost::tie( key, value, weight ) = weightTuple;

        tagMap_t::const_iterator findIt = wayTags.find( key );
        if ( findIt != wayTags.end() )
        {
            if ( findIt->second == value )
            {
                return weight;
            }
        }
    }
    
    return 1.0;
}

double WeightMapLookup::get( EdgeType e ) const
{
    typedef boost::property_map<GraphType, boost::edge_index_t>::type EdgeIdMap_t;
    EdgeIdMap_t edgeIds = boost::get( boost::edge_index, m_g );
    
    size_t edgeIndex = edgeIds[e];

    boost::shared_ptr<OSMWay> theWay = m_edgeWays.at( edgeIndex );

    double wayWeight = calculateWayWeight( theWay, m_wayWeightings );

    return m_lengths.at(edgeIndex) * wayWeight;
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

void RoutingGraph::setCycleWeights()
{
    m_routableWayKeys.push_back( "highway" );
    m_routableWayKeys.push_back( "cycleway" );
    m_routableWayKeys.push_back( "tracktype" );

    // Edge weightings for cycling. In order of precedence
    m_wayWeightings.push_back( boost::make_tuple( "cycleway", "track", 0.8 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "track", 0.8 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "cycleway", 0.8 ) );

    m_wayWeightings.push_back( boost::make_tuple( "bicycle", "yes", 1.0 ) );

    m_wayWeightings.push_back( boost::make_tuple( "highway", "bridleway", 0.9 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "byway", 0.9 ) );
    
    m_wayWeightings.push_back( boost::make_tuple( "tracktype", "grade1", 0.8 ) );
    m_wayWeightings.push_back( boost::make_tuple( "tracktype", "grade2", 0.8 ) );
    m_wayWeightings.push_back( boost::make_tuple( "tracktype", "grade3", 1.0 ) );
    m_wayWeightings.push_back( boost::make_tuple( "tracktype", "grade4", 1.1 ) );
    m_wayWeightings.push_back( boost::make_tuple( "tracktype", "grade5", 1.2 ) );

    m_wayWeightings.push_back( boost::make_tuple( "highway", "trunk", 1.3 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "trunk_link", 1.3 ) );
    
    m_wayWeightings.push_back( boost::make_tuple( "highway", "steps", 1.5 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "pedestrian", 1.5 ) );
    m_wayWeightings.push_back(boost::make_tuple(  "highway", "pedestrian", 1.5 ) );

    m_wayWeightings.push_back( boost::make_tuple( "highway", "motorway", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "motorway_link", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "bus_guideway", std::numeric_limits<double>::infinity() ) );
}

void RoutingGraph::setCarWeights()
{
    m_routableWayKeys.push_back( "highway" );

    m_wayWeightings.push_back( boost::make_tuple( "highway", "trunk", 0.8 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "trunk_link", 0.8 ) );
    
    m_wayWeightings.push_back( boost::make_tuple( "highway", "motorway", 0.7 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "motorway_link", 0.7 ) );

    m_wayWeightings.push_back( boost::make_tuple( "highway", "residential", 1.2 ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "unclassified", 1.1 ) );

    m_wayWeightings.push_back( boost::make_tuple( "highway", "track", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "footway", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "cycleway", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "bridleway", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "byway", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "steps", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "pedestrian", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "pedestrian", std::numeric_limits<double>::infinity() ) );
    m_wayWeightings.push_back( boost::make_tuple( "highway", "bus_guideway", std::numeric_limits<double>::infinity() ) );
}

RoutingGraph::RoutingGraph( const OSMFragment &frag ) : m_frag( frag ), m_nextEdgeId( 0 )
{
    // TODO: We may want to fill these from a config file (not that likely to change though...)
    //setCycleWeights();
    setCarWeights();
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


void RoutingGraph::addEdge( VertexType source, VertexType dest, double length, boost::shared_ptr<OSMWay> way )
{
    // Returns a pair: Edge descriptor, bool (true if edge added)
    EdgeType thisEdge;
    bool successfulInsert;

    const tagMap_t &wayTags = way->getTags();
    const tagMap_t::const_iterator findIt = wayTags.find( "oneway" );

    bool forward = true;
    bool backward = true;
    if ( findIt != wayTags.end() )
    {
        if ( findIt->second == "yes" ||
             findIt->second == "true" )
        {
            backward = false;
        }
        else if ( findIt->second == "-1" )
        {
            forward = false;
        }
    }

    if ( forward )
    {
        boost::tie( thisEdge, successfulInsert ) = boost::add_edge( source, dest, m_nextEdgeId++, m_graph );
        m_edgeLengths.push_back( length );
        m_edgeWays.push_back( way );
        m_edgeWayBackwards.push_back( false );
    }

    if ( backward )
    {
        boost::tie( thisEdge, successfulInsert ) = boost::add_edge( dest, source, m_nextEdgeId++, m_graph );
        m_edgeLengths.push_back( length );
        m_edgeWays.push_back( way );
        m_edgeWayBackwards.push_back( true );
    }
}
    
bool RoutingGraph::validRoutingWay( const boost::shared_ptr<OSMWay> &way )
{
    const tagMap_t &tags = way->getTags();

    BOOST_FOREACH( const std::string &routableWayKey, m_routableWayKeys )
    {
        if ( tags.find( routableWayKey ) != tags.end() )
        {
            return true;
        }
    }

    return false;
}

boost::shared_ptr<OSMNode> RoutingGraph::getNodeById( dbId_t nodeId )
{
    const OSMFragment::nodeMap_t &nodeMap = m_frag.getNodes();
    OSMFragment::nodeMap_t::const_iterator nFindIt = nodeMap.find( nodeId );
    if ( nFindIt == nodeMap.end() )
    {
        throw modosmapi::ModException( "Node not found in node map" );
    }
    return nFindIt->second;
}

void RoutingGraph::build( boost::function<void( double, double, dbId_t, bool )> routeNodeRegisterCallbackFn )
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

    BOOST_FOREACH( const nodeCountInWays_t::value_type &v, nodeCountInWays )
    {
        boost::shared_ptr<OSMNode> thisNode = getNodeById( v.first );
        
        routeNodeRegisterCallbackFn( thisNode->getLat(), thisNode->getLon(), v.first, v.second > 1 );
    }


    //EdgeWeightMapType edgeWeightMap = boost::get( boost::edge_weight, m_graph );
    // Make a routing graph edge for each relevant section of each way
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
                boost::shared_ptr<OSMNode> thisNode = getNodeById( nodeId );

                if ( lastNode )
                {
                    cumulativeDistance += distBetween(
                        lastNode->getLat(),
                        lastNode->getLon(),
                        thisNode->getLat(),
                        thisNode->getLon() );
                }
                    
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
                    m_nodeIdToWay.insert( std::make_pair( nodeId, way ) );
                }

                lastNode = thisNode;
            }
        }
    }
}


VertexType RoutingGraph::getRouteVertex( dbId_t nodeId )
{
    nodeIdToVertexMap_t::const_iterator vfindIt = m_nodeIdToVertexMap.find( nodeId );
    if ( vfindIt != m_nodeIdToVertexMap.end() )
    {
        return vfindIt->second;
    }

    nodeIdToWayMap_t::const_iterator nfindIt = m_nodeIdToWay.find( nodeId );
    if ( nfindIt == m_nodeIdToWay.end() )
    {
        throw std::runtime_error( "Node not found..." );
    }
    boost::shared_ptr<OSMWay> theWay = nfindIt->second;

    double cumulativeDistance = 0.0;
    boost::shared_ptr<OSMNode> lastNode;
    VertexType lastRouteVertex = VertexType();
    bool lastIsNewVertex = false;
    VertexType theNewVertex = VertexType();
    BOOST_FOREACH( dbId_t wayNodeId, theWay->getNodes() )
    {
        vfindIt = m_nodeIdToVertexMap.find( wayNodeId );
        bool isRoutingVertex = vfindIt != m_nodeIdToVertexMap.end();
        bool isNewVertex = wayNodeId == nodeId;

        boost::shared_ptr<OSMNode> thisNode = getNodeById( nodeId );

        if ( lastNode )
        {
            cumulativeDistance += distBetween(
                lastNode->getLat(),
                lastNode->getLon(),
                thisNode->getLat(),
                thisNode->getLon() );
        }

        if ( isRoutingVertex || isNewVertex )
        {
            VertexType thisVertex;
            if ( isRoutingVertex )
            {
                thisVertex = vfindIt->second;
            }
            else
            {
                // Make this vertex and add it to the map
                thisVertex = getVertex( nodeId );
                theNewVertex = thisVertex;
            }

            if ( isNewVertex || lastIsNewVertex )
            {
                addEdge( lastRouteVertex, thisVertex, cumulativeDistance, theWay );
            }

            lastIsNewVertex = isNewVertex;
            lastRouteVertex = thisVertex;
            cumulativeDistance = 0.0;
        }

        lastNode = thisNode;
    }

    return theNewVertex;
}

std::pair<boost::shared_ptr<OSMWay>, bool> RoutingGraph::wayFromEdge( EdgeType edge )
{
    typedef boost::property_map<GraphType, boost::edge_index_t>::type EdgeIdMap_t;
    EdgeIdMap_t edgeIds = boost::get( boost::edge_index, m_graph );
    
    size_t edgeIndex = edgeIds[edge];

    boost::shared_ptr<OSMWay> way = m_edgeWays.at( edgeIndex );
    bool wayBackwards = m_edgeWayBackwards.at( edgeIndex );
    return std::make_pair( way, wayBackwards );
}

std::pair<boost::shared_ptr<OSMWay>, bool> RoutingGraph::getWayBetween( VertexType source, VertexType dest )
{
    typedef boost::graph_traits<GraphType>::out_edge_iterator out_edge_iterator;

    out_edge_iterator it, endIt;
    for ( boost::tie( it, endIt ) = out_edges( source, m_graph ); it != endIt; it++ )
    {
        EdgeType e = *it;

        if ( target( e, m_graph ) == dest )
        {
            return wayFromEdge( e );
        }
    }
    
    throw std::runtime_error( "Failed to find edge" );
}

void RoutingGraph::getIntermediateNodes( boost::shared_ptr<OSMWay> theWay, bool wayBackwards, dbId_t fromNodeId, dbId_t toNodeId, route_t &intermediateNodes )
{
    bool output = false;
    bool appendFront = false;
    //std::cout << "WAY: " << theWay->getId() << std::endl;
    BOOST_FOREACH( dbId_t nodeId, theWay->getNodes() )
    {
        if ( !output )
        {
            if ( wayBackwards )
            {
                if ( nodeId == toNodeId )
                {
                    output = true;
                    appendFront = true;
                }
            }
            else
            {
                if ( nodeId == fromNodeId )
                {
                    output = true;
                    appendFront = false;
                }
            }
        }
        else
        {
            if ( nodeId == fromNodeId || nodeId == toNodeId )
            {
                return;
            }
            boost::shared_ptr<OSMNode> theNode = getNodeById( nodeId );
            if ( appendFront )
            {
                //std::cout << "  front: " << nodeId << std::endl;
                intermediateNodes.push_front( theNode );
            }
            else
            {
                //std::cout << "  back : " << nodeId << std::endl;
                intermediateNodes.push_back( theNode );
            }
        }
    }
}

// Assumes both nodes exists in the routing graph
void RoutingGraph::calculateRoute( dbId_t sourceNodeId, dbId_t destNodeId, route_t &route )
{
    // Get the begin and end vertices
    VertexType sourceVertex = getRouteVertex( sourceNodeId );
    VertexType destVertex   = getRouteVertex( destNodeId );
    

    NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, m_graph );
    std::vector<VertexType> p( num_vertices( m_graph ) );
    std::vector<double> d( num_vertices( m_graph ) );        

    WeightMapLookup wml( m_graph, m_edgeWays, m_edgeLengths, m_wayWeightings );

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
        bool atStart = true;
        VertexType lastVertex = VertexType();
        dbId_t lastNodeId = 0;
        typedef boost::tuple<boost::shared_ptr<OSMWay>, bool, dbId_t, dbId_t> routeSeg_t;
        std::list<routeSeg_t> routeSegs;
        for ( VertexType v = destVertex; ; v = p[v] )
        {
            NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, m_graph );
            dbId_t nodeId = nodeIndexMap[v];

            if ( !atStart )
            {
                boost::shared_ptr<OSMWay> theWay;
                bool wayBackwards;
                boost::tie( theWay, wayBackwards ) = getWayBetween( v, lastVertex );

                routeSegs.push_front( routeSeg_t( theWay, wayBackwards, nodeId, lastNodeId ) );
            }
            
            if ( p[v] == v )
            {
                break;
            }
            lastVertex = v;
            lastNodeId = nodeId;
            atStart = false;
        }

        BOOST_FOREACH( const routeSeg_t &routeSeg, routeSegs )
        {
            boost::shared_ptr<OSMWay> theWay = routeSeg.get<0>();
            bool wayBackwards = routeSeg.get<1>();
            dbId_t nodeFromId = routeSeg.get<2>();
            dbId_t nodeToId = routeSeg.get<3>();

            std::cout << "WAY" << std::endl;
            BOOST_FOREACH( const tagMap_t::value_type &v, theWay->getTags() )
            {
                std::cout << "  " << v.first << ": " << v.second << std::endl;
            }
            
            route.push_back( getNodeById( nodeFromId ) );

            route_t intermediateNodes;
            getIntermediateNodes( theWay, wayBackwards, nodeFromId, nodeToId, intermediateNodes );
            BOOST_FOREACH( boost::shared_ptr<OSMNode> interNode, intermediateNodes )
            {
                route.push_back( interNode );
            }

            route.push_back( getNodeById( nodeToId ) );
        }
    }        
}

