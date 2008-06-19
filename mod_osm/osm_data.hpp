#ifndef DATA_HPP
#define DATA_HPP

#include <string>
#include <deque>
#include <map>
#include <set>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

typedef boost::uint64_t dbId_t;
typedef std::string string_t;
typedef std::pair<std::string, std::string> tag_t;
typedef std::map<std::string, std::string> tagMap_t;
typedef boost::tuple<string_t, dbId_t, string_t> member_t;

#include "xml_reader.hpp"
#include "../testing/equality_tester.hpp"

#if 0
class ConstTagString
{
private:
    typedef std::map<std::string, size_t> stringMap_t;
    static stringMap_t m_stringToMap;
    static size_t m_lastIndex;
    
    size_t m_stringIndex;

public:
    ConstTagString( const std::string &str )
    {
        assignString( str );
    }

    void assignString( const std::string &str )
    {
        stringMap_t::iterator findIt = m_stringMap.find( str );

        if ( findIt == m_stringMap.end() )
        {
            m_stringIndex = m_lastIndex++;
            m_stringMap.insert( std::make_pair( str, m_stringIndex ) );            
        }
        else
        {
            m_stringIndex = findIt->second;
        }
    }
};

stringMap_t ConstTagString::m_stringToMap;
size_t ConstTagString::m_lastIndex = 0;
#endif

class OSMFragment;

class OSMBase
{
protected:
    dbId_t   m_id;
    string_t m_timestamp;
    string_t m_user;
    dbId_t   m_userId;

    void readBaseData( OSMFragment &frag, XMLNodeData &data );

public:
    dbId_t getId() const { return m_id; }
    const string_t &getTimeStamp() const { return m_timestamp; }
    const string_t &getUser() const { return m_user; }
    dbId_t getUserId() const { return m_userId; }
};

class OSMNode : public OSMBase
{
private:
    double             m_lat;
    double             m_lon;

    tagMap_t           m_tags;

public:
    OSMNode( OSMFragment &frag, XMLNodeData &data );

    void readTag( XMLNodeData &data );

    double getLat() const { return m_lat; }
    double getLon() const { return m_lon; }
    const tagMap_t &getTags() const { return m_tags; }
};

class OSMWay : public OSMBase
{
private:
    bool               m_visible;

    std::set<dbId_t>   m_nodes;
    tagMap_t           m_tags;

public:
    OSMWay( OSMFragment &frag, XMLNodeData &data );
    void readTag( XMLNodeData &data );
    void readNd( XMLNodeData &data );

    bool getVisible() const { return m_visible; }

    const std::set<dbId_t> &getNodes() const { return m_nodes; }
    const tagMap_t &getTags() const { return m_tags; }
};


class OSMRelation : public OSMBase
{
private:
    tagMap_t m_tags;
    std::set<member_t> m_members;

public:
    OSMRelation( OSMFragment &frag, XMLNodeData &data );
    void readMember( XMLNodeData &data );
    void readTag( XMLNodeData &data );

    const tagMap_t &getTags() const { return m_tags; }
    const std::set<member_t> &getMembers() const { return m_members; }
};

class OSMFragment
{
public:
    typedef std::map<dbId_t, boost::shared_ptr<OSMNode> >     nodeMap_t;
    typedef std::map<dbId_t, boost::shared_ptr<OSMWay> >      wayMap_t;
    typedef std::map<dbId_t, boost::shared_ptr<OSMRelation> > relationMap_t;
    typedef std::map<dbId_t, std::string>                     userMap_t;

private:
    std::string m_version;
    std::string m_generator;

    nodeMap_t     m_nodes;
    wayMap_t      m_ways;
    relationMap_t m_relations;
    userMap_t     m_userDetails;

public:
    OSMFragment() {}
    void build( XMLNodeData &data );
    void readNode( XMLNodeData &data );
    void readWay( XMLNodeData &data );
    void readRelation( XMLNodeData &data );

    void addUser( dbId_t userId, const std::string &userName );

    const std::string &getVersion() const { return m_version; }
    const std::string &getGenerator() const { return m_generator; }

    const nodeMap_t     &getNodes() const { return m_nodes; }
    const wayMap_t      &getWays() const { return m_ways; }
    const relationMap_t &getRelations() const { return m_relations; }
    const userMap_t     &getUsers() const { return m_userDetails; }
};

#endif // DATA_HPP
