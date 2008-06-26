#ifndef MODOSM_ENGINE_H
#define MODOSM_ENGINE_H

#include <iosfwd>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>

namespace modosmapi
{

class Context
{
public:
    Context()
    {
    }

    void logTime( const std::string &message )
    {
        {
            boost::posix_time::ptime t(boost::posix_time::microsec_clock::local_time());

            std::cerr << t << ": " << message << std::endl;
        }
    }
};

void map(std::ostream &opFile, // destination for output
         Context &context,      // database connection settings, etc...
         double minLat, double maxLat, double minLon, double maxLon ); // function specific parameters

} // end namespace modosmapi

#endif // MODOSM_ENGINE_H
