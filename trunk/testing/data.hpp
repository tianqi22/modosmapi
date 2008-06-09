class XMLNodeData;

typedef boost::uint64_t id_t;
typedef std::string string_t;
typedef std::pair<std::string, std::string> tag_t;
typedef std::pair<std::string, std::string> attribute_t;
typedef boost::tuple<string_t, string_t, string_t> member_t;
typedef boost::function<void(XMLNodeData &)> nodeBuildFn_t;
typedef std::map<std::string, nodeBuildFn_t> nodeBuildFnMap_t;

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

    void characters(
        const XMLCh *const chars,
        const unsigned int length );

    void warning( const xercesc::SAXParseException &e );
    void error( const xercesc::SAXParseException &e );
    void fatalError( const xercesc::SAXParseException &e );
};

XMLMemberRegistration( nodeBuildFnMap_t &nodeFnBuildMap ) : m_nodeFnBuildMap( nodeFnBuildMap )
{
}

XMLMemberRegistration &XMLMemberRegistration::operator()( const std::string &memberName, boost::function<void(XMLNodeData &)> > callBackFn )
{
    m_nodeFnBuildMap.insert( std::make_pair( memberName, callBackFn ) );

    return *this;
}

XMLNodeAttributeMap( const xercesc::Attributes &attributes ) : m_attributes( attributes )
{
    for ( int i = 0; i < attributes.getLength(); i++ )
    {
        std::string key   = attributes.getLocalName( i );
        std::string value = attributes.getValue( i );

        m_attributes.insert( std::make_pair( key, value ) );
    }
}

XMLNodeData( const xercesc::Attributes &attributes ) : m_nodeAttributeMap( attributes )
{
}

void XMLNodeData::readAttributes()
{
    return m_nodeAttributeMap;
}

void XMLNodeData::registerMembers()
{
    return XMLMemberRegistration( m_nodeFnBuildMap );
}

nodeBuildFn_t XMLNodeData::getBuildFnFor( const std::string &nodeName )
{
    nodeBuildFnMap_t::iterator findIt = m_nodeFnBuildMap.find( nodeName );
        
    if ( findIt == m_nodeFnBuildMap.end() )
    {
        // Can't find the tag - make a nice error
        throw std::exception();
    }
        
    return findIt->second;
    }
}

XMLReader::XMLReader( const XMLNodeData &startNode )
{
    m_buildStack.push_back( startNode );
}
    
void XMLReader::startElement(
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

void XMLReader::endElement(
    const XMLCh *const uri,
    const XMLCh *const localname,
    const XMLCh *const qname )
{
    m_buildStack.pop_back();
}


void XMLReader::characters(
    const XMLCh *const chars,
    const unsigned int length )
{
    // Throw an exception to complain about the characters we didn't expect
    throw std::exception();
 }

void XMLReader::warning( const xercesc::SAXParseException &e )
{
    int line = e.getLineNumber();
    std::string message( transcodeString( e.getMessage() ) );
    std::cerr << "XML parse warning (line " << line << ") " << message << std::endl;
}

void XMLReader::error( const xercesc::SAXParseException &e )
{
    int line = e.getLineNumber();
    std::string message( transcodeString( e.getMessage() ) );

    throw XmlParseException( boost::str( boost::format(
        "XML parse error (line: %d): %s" )
        % line % message ) );

}

void XMLReader::fatalError( const xercesc::SAXParseException &e )
{
     int line = e.getLineNumber();
     std::string message( transcodeString( e.getMessage() ) );

     throw XmlParseException( boost::str( boost::format(
         "XML parse error (line: %d): %s" )
         % line % message ) );
}


// IPP bit

template<typename T>
XMLNodeAttributes &operator()( const std::string &tagName, T &var )
{
    attribute_t::iterator findIt = m_attributes.find( tagName );
    if ( findIt == m_attributes.end() )
    {
        // Throw an attribute not found exception
        throw std::exception();
    }

    var = boost::lexical_cast<T>( findIt->second );

    return *this;
}
