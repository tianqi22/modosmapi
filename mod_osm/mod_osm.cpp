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

#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#include <mysql/mysql.h>

#include "test.h"

struct Sink
{
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;
    Sink (request_rec *r) : m_r (r) {}
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

typedef boost::iostreams::stream<Sink> apache_stream;


extern "C"
{
static int osm_handler(request_rec* r)
{
    if (!r->handler || strcmp(r->handler, "osm"))
    	return DECLINED;
    
    if (r->method_number != M_GET)
    	return HTTP_METHOD_NOT_ALLOWED;

    //
    // Split the query
    //
    // GET  /api/0.5/map?bbox=<left>,<bottom>,<right>,<top>
    // GET  /api/0.5/trackpoints?bbox=<left>,<bottom>,<right>,<top>&page=<pagenumber>
    // GET  /api/0.5/<objtype>/<id>/history
    // GET  /api/0.5/<objtype>s?<objtype>s=<id>[,<id>]
    // GET  /api/0.5/node/<id>/ways
    // GET  /api/0.5/<objtype>/<id>/relations
    // GET  /api/0.5/<objtype>/<id>/full
    // GET  /api/0.5/changes?hours=1&zoom=16
    // GET  /api/0.5/ways/search?type=<type>&value=<value>
    // GET  /api/0.5/relations/search?type=<type>&value=<value>
    // GET  /api/0.5/search?type=<type>&value=<value>
    // POST /api/0.5/gpx/create
    // GET  /api/0.5/gpx/<id>/details
    // GET  /api/0.5/gpx/<id>/data
    // GET  /api/0.5/user/preferences

    std::string query (r->parsed_uri.query), bbox ("bbox=");
    
    if (!query.compare (0, bbox.size (), bbox) == 0)
    {
        // TODO: fail
    }

    std::vector<std::string> args;
    std::string target (query.substr (bbox.size ()));
    boost::algorithm::split (args, target,
				boost::algorithm::is_any_of (","));

    std::vector<double> params;
    BOOST_FOREACH (const std::string &value, args)
    {
        params.push_back (boost::lexical_cast<double> (value));
    }

    if (params.size () != 4)
    {
         // TODO: fail
    }

    apache_stream as;
    as.open (Sink (r));
    
    ap_set_content_type(r, "text/html;charset=ascii");

    as << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n";

    //
    // Run the query
    //
    try
    {
        osm::Context c;
        osm::map (as, c, params [0], params [1], params [2], params [3]);
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
