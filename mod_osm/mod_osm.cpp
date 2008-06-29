#define off64_t __off64_t

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <string>

#include <iostream>
#include <fstream>
#include <exception>
#include <vector>
#include <map>
#include <iomanip>

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/assign/list_inserter.hpp>

using namespace boost::assign;

#include <mysql/mysql.h>

#include "engine.hpp"

class ModuleException
{
private:
    // TODO: Yes yes - I know this shoudn't use a std::string
    // because its methods might throw. But I'm feeling lazy...
    std::string m_message;

public:
    ModuleException( const std::string message ) : m_message( message )
    {
    }

    const std::string &getMessage() const
    {
        return m_message;
    }
};

struct ApachePageWriteSink
{
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;
    ApachePageWriteSink (request_rec *r) : m_r (r) {}
    std::streamsize write (const char *s, std::streamsize n)
    {
        // ensure s is NULL terminated
        std::string str (s, n);
        ap_rputs (str.c_str (), m_r);
        return n;
    }
private:
    request_rec *m_r;
};

typedef boost::iostreams::stream<ApachePageWriteSink> apacheStream_t;

typedef std::vector<std::string> pathComponents_t;
typedef std::map<std::string, std::string> queryComponents_t;


int notYetSupported( apacheStream_t &as, const pathComponents_t &pathComponents, const queryComponents_t &queryKVPairs );
int mapQuery( apacheStream_t &as, const pathComponents_t &pathComponents, const queryComponents_t &queryKVPairs );


void modAssert( bool predicate, std::string message )
{
    if ( !predicate )
    {
        throw ModuleException( boost::str( boost::format( "Module failed: %s" ) % message ) );
    }
}

int runQuery( apacheStream_t &as, const std::vector<std::string> &pathComponents, const std::map<std::string, std::string> &queryKVPairs )
{        
    std::string apiVersion = pathComponents[2];
    std::string queryType  = pathComponents[3];


    typedef boost::function<int(apacheStream_t &, const std::vector<std::string> &, const std::map<std::string, std::string> &)> queryFun_t;
    std::map<std::string, queryFun_t> queryFnMap;

    // TODO:
    //
    // GET  /api/0.5/<objtype>/<id>/history
    // GET  /api/0.5/<objtype>s?<objtype>s=<id>[,<id>]
    // GET  /api/0.5/<objtype>/<id>/relations
    // GET  /api/0.5/<objtype>/<id>/full


    insert( queryFnMap )
        // GET  /api/0.5/map?bbox=<left>,<bottom>,<right>,<top>
        ( "map",         &mapQuery )
        // GET  /api/0.5/trackpoints?bbox=<left>,<bottom>,<right>,<top>&page=<pagenumber>
        ( "trackpoints", &notYetSupported )
        // GET  /api/0.5/node/<id>/ways
        ( "node",        &notYetSupported )
        // GET  /api/0.5/changes?hours=1&zoom=16
        ( "changes",     &notYetSupported )
        // GET  /api/0.5/ways/search?type=<type>&value=<value>
        ( "ways",        &notYetSupported )
        // GET  /api/0.5/relations/search?type=<type>&value=<value>
        ( "relations",   &notYetSupported )
        // GET  /api/0.5/search?type=<type>&value=<value>
        ( "search",      &notYetSupported )
        // POST /api/0.5/gpx/create
        // GET  /api/0.5/gpx/<id>/details
        // GET  /api/0.5/gpx/<id>/data
        ( "gpx",         &notYetSupported )
        // GET  /api/0.5/user/preferences
        ( "user",        &notYetSupported );

    std::map<std::string, queryFun_t>::const_iterator findIt = queryFnMap.find( queryType );
    modAssert( findIt != queryFnMap.end(), "Function not found in query map" );

    return findIt->second( as, pathComponents, queryKVPairs );
}

int notYetSupported( apacheStream_t &as, const std::vector<std::string> &pathComponents, const std::map<std::string, std::string> &queryKVPairs )
{
    // TODO: fail
    return OK;
}

int mapQuery( apacheStream_t &as, const std::vector<std::string> &pathComponents, const std::map<std::string, std::string> &queryKVPairs )
{
    modAssert( queryKVPairs.find("bbox") != queryKVPairs.end(), "Map query expects bbox parameter" );
    const std::string &target = queryKVPairs.find("bbox")->second;

    std::vector<std::string> args;
    boost::algorithm::split (args, target, boost::algorithm::is_any_of (","));

    modAssert( args.size() == 4, "Map bbox parameter must have four components" );

    std::vector<double> params;
    BOOST_FOREACH (const std::string &value, args)
    {
        try
        {
            params.push_back (boost::lexical_cast<double> (value));
        }
        catch ( boost::bad_lexical_cast & )
        {
            modAssert( false, "Bbox parameters must be of floating point type" );
        }
    }

    try
    {
        modosmapi::Context c;
        modosmapi::map (as, c, params[1], params[3], params[0], params[2]);
    }
    catch (...)
    {
        modAssert( false, "Map query code failed - threw an exception" );
    }

    return OK;
}



extern "C"
{
    static int osm_handler(request_rec* r)
    {
        if (!r->handler || strcmp(r->handler, "osm"))
            return DECLINED;
    
        if (r->method_number != M_GET)
            return HTTP_METHOD_NOT_ALLOWED;

        ap_set_content_type(r, "text/html;charset=ascii");
       
        apacheStream_t as;
        as.open( ApachePageWriteSink( r ) );


        try
        {
            // Split the path into components
            std::vector<std::string> pathComponents;
            if ( r->parsed_uri.path )
            {
                // Skip leading '/'
                std::string path = r->parsed_uri.path + 1;
                boost::algorithm::split( pathComponents, path, boost::algorithm::is_any_of ("/"));
            }

            modAssert( pathComponents.size() >= 4, "Path components less than 4" );
            modAssert( pathComponents[0] == "osm" &&
                       pathComponents[1] == "api" &&
                       pathComponents[2] == "0.5",
                       boost::str( boost::format( "Path should start /osm/api/0.5 (%s, %s, %s)" )
                                   % pathComponents[0]
                                   % pathComponents[1]
                                   % pathComponents[2] ) );

        
            // Split the query string into components
            std::map<std::string, std::string> queryKVPairs;
            if ( r->parsed_uri.query )
            {
                std::vector<std::string> temp;
                boost::algorithm::split( temp, r->parsed_uri.query, boost::algorithm::is_any_of ("&"));

                BOOST_FOREACH( const std::string &kvPair, temp )
                {
                    std::vector<std::string> kvTemp;
                    boost::algorithm::split( kvTemp, kvPair, boost::algorithm::is_any_of ("="));
                    // ASSERT( kvTemp.size() == 2 );
                    if ( kvTemp.size() == 2 )
                    {
                        queryKVPairs[kvTemp[0]] = kvTemp[1];
                    }
                }
            }

            return runQuery( as, pathComponents, queryKVPairs );
        }
        catch ( const ModuleException &m )
        {
            as << m.getMessage();

            return OK;
        }
        catch ( const std::exception &e )
        {
            as << "Unidentified std::exception thrown";
            
            return OK;
        }
    }

    static void register_hooks(apr_pool_t* pool)
    {
        ap_hook_handler(osm_handler, NULL, NULL, APR_HOOK_MIDDLE);
    }

    module AP_MODULE_DECLARE_DATA osm_module = {
        STANDARD20_MODULE_STUFF,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        register_hooks
    };
}

