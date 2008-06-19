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

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/assign/list_inserter.hpp>

using namespace boost::assign;

#include <mysql/mysql.h>

#include "engine.hpp"

struct ApachePageWriteSink
{
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;
    ApachePageWriteSink (request_rec *r) : m_r (r) {}
    std::streamsize write (const char *s, std::streamsize n)
    {
        // ensure s is NUL terminated
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

static int runQuery( apacheStream_t &as, const std::vector<std::string> &pathComponents, const std::map<std::string, std::string> &queryKVPairs )
{
    // ASSERT( pathComponents.size() > 2 );
    // ASSERT( pathComponents[0] == 'api' );
        
    std::string apiVersion = pathComponents[1];
    // ASSERT( apiVersion == '0.5' );

    std::string queryType = pathComponents[2];


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
    if ( findIt == queryFnMap.end() )
    {
        // TODO: fail
    }

    return findIt->second( as, pathComponents, queryKVPairs );
}

int notYetSupported( apacheStream_t &as, const std::vector<std::string> &pathComponents, const std::map<std::string, std::string> &queryKVPairs )
{
    // TODO: fail
    return OK;
}

int mapQuery( apacheStream_t &as, const std::vector<std::string> &pathComponents, const std::map<std::string, std::string> &queryKVPairs )
{
    // TODO: Assert that the bbox param exists
    const std::string &target = queryKVPairs.find("bbox")->second;

    std::vector<std::string> args;
    boost::algorithm::split (args, target, boost::algorithm::is_any_of (","));

    std::vector<double> params;
    BOOST_FOREACH (const std::string &value, args)
    {
        params.push_back (boost::lexical_cast<double> (value));
    }

    if (params.size () != 4)
    {
        // TODO: fail
    }

    as << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n";

    //
    // Run the query
    //
    try
    {
        modosmapi::Context c;
        modosmapi::map (as, c, params [0], params [1], params [2], params [3]);
    }
    catch (...)
    {
        // TODO: fail 
    }

    //test (as);
    /*
      as << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n";
      as << "<html><head><title>How surprising!</title></head>";
      as << "<body><h1>Wow 2!</h1></body></html>";
    */
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
    
        // Split the path into components
        std::vector<std::string> pathComponents;
        boost::algorithm::split( pathComponents, r->parsed_uri.path, boost::algorithm::is_any_of ("/"));
        
        // Split the query string into components
        std::map<std::string, std::string> queryKVPairs;
        {
            std::vector<std::string> temp;
            boost::algorithm::split( temp, r->parsed_uri.query, boost::algorithm::is_any_of ("&"));

            BOOST_FOREACH( const std::string &kvPair, temp )
            {
                std::vector<std::string> kvTemp;
                boost::algorithm::split( kvTemp, kvPair, boost::algorithm::is_any_of ("="));

                // ASSERT( kvTemp.size() == 2 );
                queryKVPairs[kvTemp[0]] = kvTemp[1];
            }
        }

        ap_set_content_type(r, "text/html;charset=ascii");

        apacheStream_t as;
        as.open( ApachePageWriteSink( r ) );
        
        return runQuery( as, pathComponents, queryKVPairs );
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

