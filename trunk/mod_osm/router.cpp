# if 0

#include <boost/bind.hpp>
//#include <boost/tuple.hpp>
#include <boost/mem_fn.hpp>
#include <boost/python.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>


#include <iostream>
#include <map>
#include <cmath>

using namespace std;

typedef boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::bidirectionalS,
    boost::property<boost::vertex_name_t, int>,
    boost::property<boost::edge_weight_t, double> > GraphType;

typedef boost::graph_traits<GraphType>::vertex_descriptor VertexType;
typedef boost::graph_traits<GraphType>::edge_descriptor EdgeType;

typedef boost::property_map<GraphType, boost::edge_weight_t>::type EdgeWeightMapType;
typedef boost::property_map<GraphType, boost::vertex_name_t>::type NodeIndexMapType;


const double PI = acos( -1.0 );

// Should return the distance in km between the two points: http://jan.ucc.nau.edu/~cvm/latlon_formula.html
double distBetween( double lat1, double lon1, double lat2, double lon2 )
{
    double a1 = (lat1 / 180.0) * PI;
    double b1 = (lon1 / 180.0) * PI;
    double a2 = (lat2 / 180.0) * PI;
    double b2 = (lon2 / 180.0) * PI;

    return 6378.0 * acos(cos(a1)*cos(b1)*cos(a2)*cos(b2) + cos(a1)*sin(b1)*cos(a2)*sin(b2) + sin(a1)*sin(a2));
}

// * Iterate over ways and keep count of the number of times a node is encountered
// * Iterate over ways again, and add a routing node for each start node, end node and multiple way node
// * Ready to route!

struct NodeData
{
    int nodeId;
    double lat, lon;
    int inNumWays;

    // TODO: Don't really need to make a Vertex for all nodes. This is inefficient
    VertexType v;
   
    NodeData( int nodeId_, double lat_, double lon_, int inNumWays_, VertexType v_ ) :
        nodeId( nodeId_ ), lat( lat_ ), lon( lon_ ), inNumWays( inNumWays_ ), v( v_ )
    {
    }

    double distanceFrom( const NodeData &other ) const
    {
        return abs( distBetween( lat, lon, other.lat, other.lon ) );
    }
};

// template<class T>
// class ContainerWrapper
// {
//     T c;
//     typename T::iterator it, endIt;
// public:
//     ContainerWrapper( const ContainerWrapper<T> &other ) : c( other.c )
//     {
//         it = c.begin();
//         endIt = c.end();
//     }
//     ContainerWrapper( T &c_ ) : c( c_ )
//     {
//         it = c.begin();
//         endIt = c.end();
//     }

//     bool end() { return it == endIt; }
//     void next() { it++; }
//     typename T::iterator::value_type get() { return *it; }
// };

// typedef ContainerWrapper<vector<pair<int, double> > > DistReturnType;



class GraphBuilder
{
    map<int, NodeData *> idToNodeMap;
    GraphType g;
    VertexType lastVertex;
    NodeData *lastNode;

    int oldWayId;
    double accumulatedDistance;
public:
    GraphBuilder( SimpleDBWrapper &db );

    void makeEdge( VertexType from, VertexType to );

    void addNodeCallBack( int argc, char **argv, char **azColName );
    void addNodeWayCallBack( int argc, char **argv, char **azColName );

    GraphType &getGraph() { return g; }
    DistReturnType distFrom( int startNode );
    DistReturnType routeFrom( int startNode, int endNode );

    ~GraphBuilder();
};

// GraphBuilder::GraphBuilder( SimpleDBWrapper &db ) : lastNode( NULL ), oldWayId( -1 ), accumulatedDistance( 0.0 )
// {
//     string query;

//     // Pull in all the node data    
//     query  = "select nodes.id as nodeid, lat, lon, count(wayid) as count from nodes ";
//     query += "inner join nodesinways on nodes.id=nodesinways.nodeid group by nodeid";
//     db.runQuery( query, boost::bind( &GraphBuilder::addNodeCallBack, this, _1, _2, _3 ) );
    
//     // Connect all the important nodes up
//     query = "select wayid, nodeid from nodesinways order by wayid, id";
//     db.runQuery( query, boost::bind( &GraphBuilder::addNodeWayCallBack, this, _1, _2, _3 ) );
// }

// void GraphBuilder::addNodeCallBack( int argc, char **argv, char **azColName )
// {
//     NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, g );

//     int nodeId = boost::lexical_cast<int>( argv[0] );
//     double lat = boost::lexical_cast<double>( argv[1] );
//     double lon = boost::lexical_cast<double>( argv[2] );
//     int count = boost::lexical_cast<int>( argv[3] );

//     VertexType v = boost::add_vertex( g );
//     nodeIndexMap[v] = nodeId;

//     NodeData *n = new NodeData( nodeId, lat, lon, count, v );
//     idToNodeMap[nodeId] = n;
// }

void GraphBuilder::makeEdge( VertexType from, VertexType to )
{
    EdgeWeightMapType edgeWeightMap = boost::get( boost::edge_weight, g );

    bool inserted;
    EdgeType thisEdge;
    boost::tie(thisEdge, inserted) = boost::add_edge( from, to, g );
    edgeWeightMap[thisEdge] = accumulatedDistance;
    boost::tie(thisEdge, inserted) = boost::add_edge( to, from, g );
    edgeWeightMap[thisEdge] = accumulatedDistance;
    accumulatedDistance = 0.0;
}

void GraphBuilder::addNodeWayCallBack( int argc, char **argv, char **azColName )
{
    int nodeId = boost::lexical_cast<int>( argv[1] );
    int wayId = boost::lexical_cast<int>( argv[0] );

    if ( idToNodeMap.find( nodeId ) != idToNodeMap.end() )
    {
        NodeData *thisNode = idToNodeMap[nodeId];
        if ( wayId == oldWayId )
        {
            accumulatedDistance += lastNode->distanceFrom( *thisNode );
            if ( thisNode->inNumWays > 1 )
            {
                makeEdge( lastVertex, thisNode->v );
                lastVertex = thisNode->v;
            }
        }
        else 
        {
            if ( oldWayId != -1 )
            {
                makeEdge( lastVertex, lastNode->v );
            }
            lastVertex = thisNode->v;
        }
        oldWayId = wayId;
        lastNode = thisNode;
    }
}


DistReturnType GraphBuilder::distFrom( int startNode )
{
    EdgeWeightMapType edgeWeightMap = boost::get( boost::edge_weight, g );
    NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, g );

    VertexType start = idToNodeMap[startNode]->v;

    std::vector<VertexType> p( boost::num_vertices(g) );
    std::vector<double> d( boost::num_vertices(g) );
    
    boost::dijkstra_shortest_paths( g, start, boost::predecessor_map( &p[0] ).distance_map( &d[0] ) );

    vector<pair<int, double> > c;
    boost::graph_traits <GraphType>::vertex_iterator vi, vend;
    for ( boost::tie(vi, vend) = vertices(g); vi != vend; ++vi)
    {
        int nodeId = nodeIndexMap[*vi];
        double distance = d[*vi];

        if ( distance < 100000.0 )
        {
            c.push_back( make_pair( nodeId, distance ) );
        }
    }

    return DistReturnType( c );
}

DistReturnType GraphBuilder::routeFrom( int startNode, int endNode )
{
    EdgeWeightMapType edgeWeightMap = boost::get( boost::edge_weight, g );
    NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, g );

    VertexType start = idToNodeMap[startNode]->v;
    VertexType end = idToNodeMap[endNode]->v;

    std::vector<VertexType> p( boost::num_vertices(g) );
    std::vector<double> d( boost::num_vertices(g) );

    boost::dijkstra_shortest_paths( g, start, boost::predecessor_map( &p[0] ).distance_map( &d[0] ) );

    vector<pair<int, double> > nodePathList;

    VertexType iter = end;
    while ( iter != start )
    {
        int nodeId = nodeIndexMap[iter];
        double distance = d[iter];

        nodePathList.push_back( make_pair( nodeId, distance ) );
        
        iter = p[iter];
    }
    nodePathList.push_back( make_pair( nodeIndexMap[start], 0.0 ) );

    return DistReturnType( nodePathList );
}

GraphBuilder::~GraphBuilder()
{
    // Delete the temporary structure
    typedef pair<int, NodeData *> p_t;
    BOOST_FOREACH( p_t p, idToNodeMap )
    {
        delete p.second;
    }
}


class Tester
{
    SimpleDBWrapper db;
    GraphBuilder gb;
    boost::graph_traits<GraphType>::edge_iterator it, endIt;
public:
    Tester() : db( "mapdata/oxtestdata.db" ), gb( db )
    {
        tie( it, endIt ) = boost::edges( gb.getGraph() );
    }

    bool end() { return it == endIt; }
    
    pair<int, int> get()
    {
        NodeIndexMapType nodeIndexMap = boost::get( boost::vertex_name, gb.getGraph() );

        return make_pair(
            nodeIndexMap[ boost::source( *it, gb.getGraph() ) ],
            nodeIndexMap[ boost::target( *it, gb.getGraph() ) ] );
    }

    void next() { it++; }

    DistReturnType distFrom( int nodeId ) { return gb.distFrom( nodeId ); }
    DistReturnType routeFrom( int startNode, int endNode ) { return gb.routeFrom( startNode, endNode ); }
};

BOOST_PYTHON_MODULE( Tester )
{
    class_<Tester>( "Tester" )
        .def( "end", &Tester::end )
        .def( "get", &Tester::get )
        .def( "next", &Tester::next )
        .def( "distFrom", &Tester::distFrom )
        .def( "routeFrom", &Tester::routeFrom );

    class_<pair<int, int> >( "IntEdgePair" )
        .def_readonly( "first", &pair<int, int>::first )
        .def_readonly( "second", &pair<int, int>::second );

    class_<pair<int, double> >( "NodeDistPair" )
        .def_readonly( "nodeid", &pair<int, double>::first )
        .def_readonly( "dist", &pair<int, double>::second );

    class_<DistReturnType>( "DistInfo", no_init )
        .def( "end", &DistReturnType::end )
        .def( "next", &DistReturnType::next )
        .def( "get", &DistReturnType::get );
}


/*int main( int argc, char **argv )
{
    SimpleDBWrapper db( "mapdata/oxtestdata.db" );

    GraphBuilder gb( db );

    int startNode = boost::lexical_cast<int>( argv[1] );
    int endNode = boost::lexical_cast<int>( argv[2] );

    gb.findRoute( startNode, endNode );
}*/


#endif
