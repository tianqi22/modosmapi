#ifndef OSM_TEST_H
#define OSM_TEST_H

#include <iosfwd>

namespace osm
{

class Context {};

void map(std::ostream &opFile, // destination for output
         Context &context,      // database connection settings, etc...
         double minLat, double maxLat, double minLon, double maxLon ); // function specific parameters

} // end namespace osm

#endif // OSM_TEST_H
