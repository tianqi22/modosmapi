#include <cmath>


const double PI = acos( -1.0 );

// Should return the distance in km between the two points
// Ref: http://jan.ucc.nau.edu/~cvm/latlon_formula.html
double distBetween( double lat1, double lon1, double lat2, double lon2 )
{
    using namespace std;

    double a1 = (lat1 / 180.0) * PI;
    double b1 = (lon1 / 180.0) * PI;
    double a2 = (lat2 / 180.0) * PI;
    double b2 = (lon2 / 180.0) * PI;

    return 6378.0 * acos(cos(a1)*cos(b1)*cos(a2)*cos(b2) + cos(a1)*sin(b1)*cos(a2)*sin(b2) + sin(a1)*sin(a2));
}
