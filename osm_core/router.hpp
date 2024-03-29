#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <boost/graph/adjacency_list.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>

#include <vector>
#include <string>


// Tag: key, value, length multiplier. Can have +inf
typedef boost::tuple<ConstTagString, ConstTagString, double> wayWeighting_t;
typedef std::vector<wayWeighting_t> wayWeightings_t;


typedef boost::adjacency_list<
    // Both edges and all vertices stored in vectors
    boost::vecS,
    boost::vecS,
    // Access to both input and output edges, in a directed graph
    boost::bidirectionalS,
    //boost::undirectedS,
    boost::property<boost::vertex_name_t, int>,
    boost::property<boost::edge_index_t, size_t> > GraphType;

typedef boost::graph_traits<GraphType>::vertex_descriptor VertexType;
typedef boost::graph_traits<GraphType>::edge_descriptor EdgeType;

typedef boost::property_map<GraphType, boost::edge_weight_t>::type EdgeWeightMapType;
typedef boost::property_map<GraphType, boost::vertex_name_t>::type NodeIndexMapType;


typedef std::map<boost::uint64_t, VertexType> nodeIdToVertexMap_t;

class RoutingGraph
{
public:
    typedef std::list<boost::shared_ptr<OSMNode> > route_t;

    /* Member data */
    /***************/
private:
    const OSMFragment&                           m_frag;
    nodeIdToVertexMap_t                          m_nodeIdToVertexMap;
    GraphType                                    m_graph;

    std::vector<std::string>                     m_routableWayKeys;
    wayWeightings_t                              m_wayWeightings;

    size_t                                       m_nextEdgeId;
    std::vector<double>                          m_edgeLengths;
    std::vector<boost::shared_ptr<OSMWay> >      m_edgeWays;
    std::vector<bool>                            m_edgeWayBackwards;

    // For nodes on only one (routing) way: get the way from the node id
    typedef std::map<dbId_t, boost::shared_ptr<OSMWay> > nodeIdToWayMap_t;
    nodeIdToWayMap_t m_nodeIdToWay;

public:
    RoutingGraph( const OSMFragment &frag );
    VertexType getVertex( boost::uint64_t nodeId );
    void addEdge( VertexType source, VertexType dest, double length, boost::shared_ptr<OSMWay> way );
    bool validRoutingWay( const boost::shared_ptr<OSMWay> &way );
    void build( boost::function<void( double, double, dbId_t, bool )> routeNodeRegisterCallback );
    void calculateRoute( dbId_t sourceNodeId, dbId_t destNodeId, route_t &route );

    VertexType getRouteVertex( dbId_t nodeId );

private:
    boost::shared_ptr<OSMNode> getNodeById( dbId_t nodeId );
    std::pair<boost::shared_ptr<OSMWay>, bool> wayFromEdge( EdgeType edge );
    std::pair<boost::shared_ptr<OSMWay>, bool> getWayBetween( VertexType source, VertexType dest );
    void getIntermediateNodes( boost::shared_ptr<OSMWay> theWay, bool wayBackwards, dbId_t lastNodeId, dbId_t nodeId, route_t &intermediateNodes );

    void setCycleWeights();
    void setCarWeights();
};


#endif // ROUTER_HPP
