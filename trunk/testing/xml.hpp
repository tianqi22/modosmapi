#ifndef XML_HPP
#define XML_HPP

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>


std::string escapeChars( std::string toEscape );
std::string transcodeString( const XMLCh *const toTranscode );

class XmlParseException : public std::exception
{
    std::string m_message;

public:
    XmlParseException( const std::string &message ) : m_message( message ) { }
    virtual ~XmlParseException() throw () { }
};


class XMLNodeData;

typedef boost::function<void(XMLNodeData &)> nodeBuildFn_t;
typedef std::map<std::string, nodeBuildFn_t> nodeBuildFnMap_t;

class XMLMemberRegistration
{
private:
    nodeBuildFnMap_t m_nodeFnBuildMap;

public:
    XMLMemberRegistration( nodeBuildFnMap_t &nodeFnBuildMap );
    XMLMemberRegistration &operator()( const std::string &memberName, nodeBuildFn_t callBackFn );
};


class XMLNodeAttributeMap
{
private:
    attributeMap_t m_attributes;

public:
    XMLNodeAttributeMap() {}
    XMLNodeAttributeMap( const xercesc::Attributes &attributes );

    template<typename T>
    XMLNodeAttributeMap &operator()( const std::string &tagName, T &var );
};


class XMLNodeData
{
private:
    XMLNodeAttributeMap   m_nodeAttributeMap;
    XMLMemberRegistration m_memberRegistration;
    nodeBuildFnMap_t      m_nodeFnBuildMap;

public:
    XMLNodeData();
    XMLNodeData( const xercesc::Attributes &attributes );
    XMLNodeAttributeMap &readAttributes();
    XMLMemberRegistration &registerMembers();
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


// IPP bit

template<typename T>
XMLNodeAttributeMap &XMLNodeAttributeMap::operator()( const std::string &tagName, T &var )
{
    attributeMap_t::iterator findIt = m_attributes.find( tagName );
    if ( findIt == m_attributes.end() )
    {
        // Throw an attribute not found exception
        throw std::exception();
    }

    var = boost::lexical_cast<T>( findIt->second );

    return *this;
}

#endif // XML_HPP
