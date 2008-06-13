#ifndef DATA_HPP
#define DATA_HPP

#include <string>
#include <deque>
#include <map>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

typedef boost::uint64_t dbId_t;
typedef std::string string_t;
typedef std::pair<std::string, std::string> tag_t;
typedef std::map<std::string, std::string> tagMap_t;
typedef boost::tuple<string_t, string_t, string_t> member_t;

#include "xml_reader.hpp"


class OSMFragment;

class OSMNode
{
private:
    dbId_t             m_id;
    double             m_lat;
    double             m_lon;
    string_t           m_timestamp;
    string_t           m_user;
    dbId_t             m_userId;

    tagMap_t           m_tags;

public:
    OSMNode( OSMFragment &frag, XMLNodeData &data );

    dbId_t getId() const { return m_id; }
    void   readTag( XMLNodeData &data );
    const tagMap_t &getTags() const { return m_tags; }
};

class OSMWay
{
private:
    dbId_t               m_id;
    bool               m_visible;
    string_t           m_timestamp;
    string_t           m_user;
    dbId_t               m_userId;

    std::vector<dbId_t>  m_nodes;
    tagMap_t           m_tags;

public:
    OSMWay( OSMFragment &frag, XMLNodeData &data );
    void readTag( XMLNodeData &data );
    void readNd( XMLNodeData &data );

    dbId_t getId() const { return m_id; }
    bool getVisible() const { return m_visible; }
    const string_t &getTimeStamp() { return m_timestamp; }
    const string_t &getUser() { return m_user; }

    const std::vector<dbId_t> &getNodes() const { return m_nodes; }
    const tagMap_t &getTags() const { return m_tags; }
};

class OSMRelation
{
private:
    dbId_t   m_id;
    string_t m_timestamp;
    string_t m_user;
    dbId_t   m_userId;

    tagMap_t m_tags;
    std::vector<member_t> m_members;

public:
    OSMRelation( OSMFragment &frag, XMLNodeData &data );
    void readMember( XMLNodeData &data );
    void readTag( XMLNodeData &data );

    dbId_t getId() const { return m_id; }
    const tagMap_t &getTags() const { return m_tags; }
    const std::vector<member_t> &getMembers() const { return m_members; }
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
