#ifndef MODOSM_ENGINE_H
#define MODOSM_ENGINE_H

#include <iosfwd>

namespace modosm
{

class Context {};

void map(std::ostream &opFile, // destination for output
         Context &context,      // database connection settings, etc...
         double minLat, double maxLat, double minLon, double maxLon ); // function specific parameters

} // end namespace modosm

#endif // MODOSM_ENGINE_H
