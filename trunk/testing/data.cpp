
class XMLMemberRegistration
{
private:
    nodeBuildFnMap_t m_nodeFnBuildMap;

public:
    XMLMemberRegistration( nodeBuildFnMap_t &nodeFnBuildMap );
    XMLMemberRegistration &operator()( const std::string &memberName, boost::function<void(XMLNodeData &)> > callBackFn );
};


class XMLNodeAttributeMap
{
private:
    std::map<std::string, std::string> m_attributes;

public:
    XMLNodeAttributeMap( const xercesc::Attributes &attributes );

    template<typename T>
    XMLNodeAttributes &operator()( const std::string &tagName, T &var );
};


class XMLNodeData
{
private:
    XMLNodeAttributeMap m_nodeAttributeMap;
    nodeBuildFnMap_t    m_nodeFnBuildMap;

public:
    XMLNodeData( const xercesc::Attributes &attributes );
    void readAttributes();
    void registerMembers();
    nodeBuildFn_t getBuildFnFor( const std::string &nodeName );
};

class XMLReader : public xercesc::DefaultHandler
{
private:
    std::deque<XMLNodeData> m_buildStack;

public:
    XMLReader( const XMLNodeData &startNode );

    void startElement(
        const XMLCh *const uri,
        const XMLCh *const localname,
        const XMLCh *const qame,
        const xercesc::Attributes &attributes );

    void endElement(
        const XMLCh *const uri,
        const XMLCh *const localname,
        const XMLCh *const qame );


    XMLReader( const XMLNodeData &startNode )
    {
        m_buildStack.push_back( startNode );
    }
    
    void startElement(
        const XMLCh *const uri,
        const XMLCh *const localname,
        const XMLCh *const qame,
        const xercesc::Attributes &attributes )
    {
        std::string tagName = transcodeString( localname );
        nodeBuildFn_t buildFn = m_buildStack.back().getBuildFnFor( tagName );

        m_buildStack.push_back( XMLNodeData( attributes ) );
        buildFn( m_buildStack.back() );
    }

    void endElement(
        const XMLCh *const uri,
        const XMLCh *const localname,
        const XMLCh *const qname )
    {
        m_buildStack.pop_back();
    }

    void characters(
        const XMLCh *const chars,
        const unsigned int length )
    {
        // Throw an exception to complain about the characters we didn't expect
        throw std::exception();
    }

    void warning( const xercesc::SAXParseException &e )
    {
        int line = e.getLineNumber();
        std::string message( transcodeString( e.getMessage() ) );
        std::cerr << "XML parse warning (line " << line << ") " << message << std::endl;
    }

    void error( const xercesc::SAXParseException &e )
    {
        int line = e.getLineNumber();
        std::string message( transcodeString( e.getMessage() ) );

        throw XmlParseException( boost::str( boost::format(
            "XML parse error (line: %d): %s" )
            % line % message ) );

    }

    void fatalError( const xercesc::SAXParseException &e )
    {
        int line = e.getLineNumber();
        std::string message( transcodeString( e.getMessage() ) );

        throw XmlParseException( boost::str( boost::format(
            "XML parse error (line: %d): %s" )
            % line % message ) );
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