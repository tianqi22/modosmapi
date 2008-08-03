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
#include <boost/date_time/posix_time/posix_time.hpp>

#include "utils.hpp"

typedef boost::uint64_t dbId_t;
typedef std::string string_t;
typedef std::pair<ConstTagString, ConstTagString> tag_t;
typedef std::map<ConstTagString, ConstTagString> tagMap_t;
typedef boost::tuple<string_t, dbId_t, string_t> member_t;

#include "xml_reader.hpp"
#include "../testing/equality_tester.hpp"

class OSMFragment;

class OSMBase
{
protected:
    dbId_t   m_id;
    boost::posix_time::ptime m_timestamp;
    string_t m_user;
    dbId_t   m_userId;

    void readBaseData( OSMFragment &frag, XMLNodeData &data );

public:
    dbId_t getId() const { return m_id; }
    const boost::posix_time::ptime &getTimeStamp() const { return m_timestamp; }
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
    bool                m_visible;

    std::vector<dbId_t> m_nodes;
    tagMap_t            m_tags;

public:
    OSMWay( OSMFragment &frag, XMLNodeData &data );
    void readTag( XMLNodeData &data );
    void readNd( XMLNodeData &data );

    bool getVisible() const { return m_visible; }

    const std::vector<dbId_t> &getNodes() const { return m_nodes; }
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
    void readBounds( XMLNodeData &data );

    void addUser( dbId_t userId, const std::string &userName );

    const std::string &getVersion() const { return m_version; }
    const std::string &getGenerator() const { return m_generator; }

    const nodeMap_t     &getNodes() const { return m_nodes; }
    const wayMap_t      &getWays() const { return m_ways; }
    const relationMap_t &getRelations() const { return m_relations; }
    const userMap_t     &getUsers() const { return m_userDetails; }
};

#endif // DATA_HPP
