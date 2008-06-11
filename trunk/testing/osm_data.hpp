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
typedef boost::tuple<string_t, string_t, string_t> member_t;

#include "xml_reader.hpp"

class OSMNode
{
private:
    id_t               m_id;
    boost::int64_t     m_lat;
    boost::int64_t     m_lon;
    string_t           m_timestamp;
    string_t           m_user;
    id_t               m_userId;

    std::vector<tag_t> m_tags;

public:
    OSMNode( XMLNodeData &data );
    void readTag( XMLNodeData &data );
};

class OSMWay
{
private:
    id_t               m_id;
    string_t           m_timestamp;
    string_t           m_user;
    id_t               m_userId;

    std::vector<id_t> m_nodes;
    std::vector<tag_t> m_tags;

public:
    OSMWay( XMLNodeData &data );
    void readTag( XMLNodeData &data );
    void readNd( XMLNodeData &data );
};

class OSMRelation
{
private:
    id_t m_id;
    string_t m_timestamp;
    string_t m_user;
    id_t m_userId;

    std::vector<tag_t> m_tags;
    std::vector<member_t> m_members;

public:
    OSMRelation( XMLNodeData &data );
    void readMember( XMLNodeData &data );
    void readTag( XMLNodeData &data );
};

class OSMFragment
{
private:
    std::string m_version;
    std::string m_generator;

    std::vector<boost::shared_ptr<OSMNode> >     m_nodes;
    std::vector<boost::shared_ptr<OSMWay> >      m_ways;
    std::vector<boost::shared_ptr<OSMRelation> > m_relations;

public:
    OSMFragment() {}
    void build( XMLNodeData &data );
    void readNode( XMLNodeData &data );
    void readWay( XMLNodeData &data );
    void readRelation( XMLNodeData &data );
};

#endif // DATA_HPP
