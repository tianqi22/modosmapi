#include "xml_reader.hpp"
#include "osm_data.hpp"
#include "dbhandler.hpp"
#include "equality_tester.hpp"


#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

void compareTags( const tagMap_t &lhs, const tagMap_t &rhs, const EqualityTester &tester )
{
    tester.requireEqual( lhs.size(), rhs.size(), "Number of tags do not match" );
    BOOST_FOREACH( const tagMap_t::value_type &v, lhs )
    {
        tagMap_t::const_iterator findIt = rhs.find( v.first );

        if ( findIt == rhs.end() )
        {
            tester.error( boost::str( boost::format( "Tag %s missing in rhs" ) % v.first ) );
        }
        
        tester.requireEqual( v.second, findIt->second, "Tag values do not match" );
    }
}

void compareBase( const OSMBase &lhs, const OSMBase &rhs, const EqualityTester &tester )
{
    tester.requireEqual( lhs.getId(), rhs.getId(), "Ids do not match" );
    tester.requireEqual( lhs.getTimeStamp(), rhs.getTimeStamp(), "Timestamps do not match" );
    //tester.requireEqual( lhs.getUser(), rhs.getUser(), "User names do not match" );
    tester.requireEqual( lhs.getUserId(), rhs.getUserId(), "User ids do not match" );
}

void compare( const OSMNode &lhs, const OSMNode &rhs, const EqualityTester &tester )
{
    compareBase( lhs, rhs, tester );

    tester.requireEqual( lhs.getLat(), rhs.getLat(), "Latitude does not match" );
    tester.requireEqual( lhs.getLon(), rhs.getLon(), "Longitude does not match" );

    compareTags( lhs.getTags(), rhs.getTags(), tester );
}

void compare( const OSMWay &lhs, const OSMWay &rhs, const EqualityTester &tester )
{
    compareBase( lhs, rhs, tester );

    tester.requireEqual( lhs.getVisible(), rhs.getVisible(), "Visibility does not match" );

    compareTags( lhs.getTags(), rhs.getTags(), tester );

    std::vector<dbId_t> diffs;

    std::set_symmetric_difference(
        lhs.getNodes().begin(), lhs.getNodes().end(),
        rhs.getNodes().begin(), rhs.getNodes().end(),
        std::back_inserter( diffs ) );

    if ( !diffs.empty() )
    {
        BOOST_FOREACH( dbId_t idDiff, diffs )
        {
            tester.error( boost::str( boost::format( "Node id %d only present in one file" ) % idDiff ) );
        }
    }
}

void compare( const OSMRelation &lhs, const OSMRelation &rhs, const EqualityTester &tester )
{
    compareBase( lhs, rhs, tester );

    compareTags( lhs.getTags(), rhs.getTags(), tester );


    std::vector<member_t> diffs;
    std::set_symmetric_difference(
        lhs.getMembers().begin(), lhs.getMembers().end(),
        rhs.getMembers().begin(), rhs.getMembers().end(),
        std::back_inserter( diffs ) );

    if ( !diffs.empty() )
    {
        BOOST_FOREACH( member_t memberDiff, diffs )
        {
            tester.error( boost::str( boost::format( "Member (%s, %d, %s) only present in one file" )
                    % memberDiff.get<0>()
                    % memberDiff.get<1>()
                    % memberDiff.get<2>() )  );
        }
    }
}


void compareOSMXML( const OSMFragment &frag1, const OSMFragment &frag2 )
{
    EqualityTester tester( "OSM" );

    tester.requireEqual( frag1.getVersion(), frag2.getVersion(), "Versions do not match" );
    tester.requireEqual( frag1.getGenerator(), frag2.getGenerator(), "Generators do not match" );
    tester.requireEqual( frag1.getNodes().size(), frag2.getNodes().size(), "Number of nodes does not match" );
    tester.requireEqual( frag1.getWays().size(), frag2.getWays().size(), "Number of ways does not match" );
    tester.requireEqual( frag1.getRelations().size(), frag2.getRelations().size(), "Number of relations does not match" );

    BOOST_FOREACH( const OSMFragment::nodeMap_t::value_type &v, frag1.getNodes() )
    {
        EqualityTester nodeTester = tester.context( boost::str( boost::format( "Node %d" ) % v.first ) );

        OSMFragment::nodeMap_t::const_iterator findIt = frag2.getNodes().find( v.first );

        if ( findIt == frag2.getNodes().end() )
        {
            nodeTester.error( "Missing in second file" );
        }
        else
        {
            compare( *v.second, *findIt->second, nodeTester );
        }
    }
  
    BOOST_FOREACH( const OSMFragment::wayMap_t::value_type &v, frag1.getWays() )
    {
        EqualityTester wayTester = tester.context( boost::str( boost::format( "Way %d" ) % v.first ) );

        OSMFragment::wayMap_t::const_iterator findIt = frag2.getWays().find( v.first );

        if ( findIt == frag2.getWays().end() )
        {
            wayTester.error( "Missing in second file" );
        }
        else
        {
            compare( *v.second, *findIt->second, wayTester );
        }
    }

    BOOST_FOREACH( const OSMFragment::relationMap_t::value_type &v, frag1.getRelations() )
    {
        EqualityTester relationTester = tester.context( boost::str( boost::format( "Relation %d" ) % v.first ) );

        OSMFragment::relationMap_t::const_iterator findIt = frag2.getRelations().find( v.first );

        if ( findIt == frag2.getRelations().end() )
        {
            relationTester.error( "Missing in second file" );
        }
        else
        {
            compare( *v.second, *findIt->second, relationTester );
        }
    }
}

void readOSMXML( XercesInitWrapper &x, const std::string &fileName, OSMFragment &frag )
{
    std::cout << "Reading XML file: " << fileName << std::endl;

    xercesc::SAX2XMLReaderImpl &parser = x.getParser();

    boost::shared_ptr<XMLNodeData> startNdData( new XMLNodeData() );

    startNdData->registerMembers()( "osm", boost::bind( &OSMFragment::build, &frag, _1 ) );

    XMLReader handler( startNdData );

    parser.setContentHandler( &handler );
    parser.setErrorHandler( &handler );

    parser.parse( fileName.c_str() );

    std::cout << "Done..." << std::endl;
}


int main( int argc, char **argv )
{
    if ( argc != 3 )
    {
        std::cout << "Incorrect number of arguments" << std::endl;
        return -1;
    }

    std::string file1 = argv[1];
    std::string file2 = argv[2];

    try
    {
        XercesInitWrapper x;

        OSMFragment fragment1, fragment2;
         
        readOSMXML( x, file1, fragment1 );
        readOSMXML( x, file2, fragment2 );

        compareOSMXML( fragment1, fragment2 );
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
