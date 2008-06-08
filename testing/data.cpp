
typedef boost::uint64_t id_t;
typedef std::string string_t;
typedef std::pair<std::string, std::string> tag_t;
typedef boost::tuple<string_t, string_t, string_t> member_t;

class XMLNodeAttributes
{
public:
    template<typename T>
    XMLNodeAttributes &operator()( const std::string &tagName, T &var )
    {
        // Bring this in from the map, then lexical cast it into var

        return *this;
    }
};

class XMLMemberRegistration
{
public:
    XMLMemberRegistration &operator()( const std::string &memberName, boost::function<void(XMLNodeData &)> > callBackFn )
    {
        // Pop something on the stack

        return *this;
    }
};

class XMLNodeData
{
public:
    void readAttributes()
    {
        return XMLNodeAttributes( /* Appropriate context here */ );
    }

    void registerMembers()
    {
        return XMLMemberRegistration( /* Appropriate context here */ );
    }
};

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
    void readTag( data )
    {
        string_t k, v;
        data.readAttributes()( "k", k )( "v", v );

        m_tags.push_back( tag_t( k, v ) );
    }

    void readNode( data )
    {
        data.readAttributes()
            ( "id", m_id )
            ( "lat", m_lat )
            ( "lon", m_lon )
            ( "timestamp", m_timestamp )
            ( "user", m_user )
            ( "uid", m_userId );

        data.registerMembers()("tag", boost::bind( readTag, this, _1 ) );
    }
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
    void readTag( data )
    {
        string_t k, v;
        data.readAttributes()( "k", k )( "v", v );
        m_tags.push_back( tag_t( k, v ) );
    }

    void readNd( data )
    {
        id_t ndId;
        data.readAttributes()( "ref", nId );
        m_nodes.push_back( nId );
    }

    void readWay( data )
    {
        data.readAttributes()
            ( "id", m_id )
            ( "timestamp", m_timestamp )
            ( "user", m_user )
            ( "uid", m_userId );

        data.registerMembers()
            ( "nd", boost::bind( readNd, this, _1 ) )
            ( "tag", boost::bind( readTag, this, _1 ) );
    }
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
    void readMember( data )
    {
        member_t theMember;
        data.readAttributes()
            ( "type", theMember.get<0>() )
            ( "ref", theMember.get<1>() )
            ( "role", theMember.get<2>() );
        m_members.push_back( theMember );
    }

    void readTag( data )
    {
        tag_t theTag;
        data.readAttributes()
            ( "k", theMember.get<0>() )
            ( "v", theMember.get<1>() );
    }

    void readRelation( data )
    {
        data.readAttributes()
            ( "id", m_id )
            ( "timestamp", m_timestamp )
            ( "user", m_user )
            ( "uid", m_userId );

        data.registerMembers()
            ( "member", boost::bind( readMember, this, _1 ) )
            ( "tag", boost::bind( readTag, this, _1 ) );
    }
};


// relation: id, timestamp, user, uid
//   member: type, ref, role
//   tag: k, v
