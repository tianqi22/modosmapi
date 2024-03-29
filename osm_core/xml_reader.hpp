#ifndef XML_HPP
#define XML_HPP

#include <string>
#include <map>
#include <deque>
#include <iostream>

#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/parsers/SAX2XMLReaderImpl.hpp>
#include <xercesc/util/BinInputStream.hpp>

typedef std::map<std::string, std::string> attributeMap_t;

std::string escapeChars( std::string toEscape );
std::string transcodeString( const XMLCh *const toTranscode );

class XmlParseException : public std::exception
{
    std::string m_message;

public:
    XmlParseException( const std::string &message ) : m_message( message ) { }
    virtual ~XmlParseException() throw () { }
    virtual const char* what() const throw () { return m_message.c_str(); }
};


class XMLNodeData;

typedef boost::function<void(XMLNodeData &)> nodeBuildFn_t;
typedef std::map<std::string, nodeBuildFn_t> nodeBuildFnMap_t;

class XMLMemberRegistration
{
private:
    nodeBuildFnMap_t &m_nodeFnBuildMap;

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
    XMLNodeAttributeMap &operator()( const std::string &tagName, T &var, bool optional=false, T defaultValue=T() );
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

    void dumpRegMap()
    {
        nodeBuildFnMap_t::iterator it;
        for ( it = m_nodeFnBuildMap.begin(); it != m_nodeFnBuildMap.end(); it++ )
        {
            std::cout << "Member: " << it->first << std::endl;
        }
    }

    ~XMLNodeData()
    {
    }
};

class XMLReader : public xercesc::DefaultHandler
{
private:
    std::deque<boost::shared_ptr<XMLNodeData> > m_buildStack;

public:
    XMLReader( boost::shared_ptr<XMLNodeData> startNode );

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

class XercesInitWrapper
{
    xercesc::SAX2XMLReaderImpl *m_parser;

public:
    XercesInitWrapper();
    ~XercesInitWrapper();

    xercesc::SAX2XMLReaderImpl &getParser();
};

// IPP bit

template<typename T>
void extended_lexical_cast( const std::string &val, T &var )
{
    var = boost::lexical_cast<T>( val );
}

void extended_lexical_cast( const std::string &val, bool &var );
void extended_lexical_cast( const std::string &val, boost::posix_time::ptime &var );

template<typename T>
XMLNodeAttributeMap &XMLNodeAttributeMap::operator()( const std::string &tagName, T &var, bool optional, T defaultValue )
{
    attributeMap_t::iterator findIt = m_attributes.find( tagName );
    if ( findIt == m_attributes.end() )
    {
        // Throw an attribute not found exception
        if ( optional )
        {
            var = defaultValue;
        }
        else
        {
            throw XmlParseException( "Error: missing attribute: " + tagName );
        }
    }
    else
    {
        try
        {
            extended_lexical_cast( findIt->second, var );
        }
        catch ( std::exception &e )
        {
            std::cerr << "Trying to cast tag: " << tagName << " with value " << findIt->second;
            std::cerr << " to type " << typeid( var ).name() << std::endl;
            throw ;
        }
    }

    return *this;
}

class OSMFragment;

void readOSMXML( XercesInitWrapper &x, const std::string &fileName, OSMFragment &frag );


class StreamIS : public xercesc::InputSource
{
private:
    class SBIS : public xercesc::BinInputStream
    {
    public:
        SBIS( std::istream &is ) : m_is(is), m_curPos(0)
        {
        }
        virtual unsigned int readBytes( XMLByte *const buf, const unsigned int bufSize )
        {
            try
            {
                m_is.read( reinterpret_cast<char * const>( buf ), bufSize );
                size_t readCount = m_is.gcount();
                m_curPos += readCount;
                return readCount;
            }
            catch(...)
            {
                throw std::runtime_error( "Error in reading from stream" );
            }
        }

        virtual unsigned int curPos() const
        {
            return m_curPos;
        }

    private:
        std::istream &m_is;
        size_t m_curPos;
    };

public:
    StreamIS( std::istream &is ) : m_is( is )
    {
    }

    virtual xercesc::BinInputStream *makeStream() const
    {
        return new SBIS( m_is );
    }

private:
    std::istream &m_is;
};

#endif // XML_HPP
