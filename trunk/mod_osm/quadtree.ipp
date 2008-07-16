
#include <boost/foreach.hpp>


template<typename CoordType>
bool RectangularRegion<CoordType>::inRegion( const XYPoint<CoordType> &point ) const
{
    return point.m_x >= m_minMin.m_x && point.m_x <= m_maxMax.m_x &&
           point.m_y >= m_minMin.m_y && point.m_y <= m_maxMax.m_y;
}

template<typename CoordType>
bool RectangularRegion<CoordType>::overlaps( const RectangularRegion<CoordType> &rhs ) const
{
    // TODO: This can't be efficient, but it does the job for now
    bool cornerlessOverlap =
        m_minMin.m_x >= rhs.m_minMin.m_x && m_maxMax.m_x <= rhs.m_maxMax.m_x &&
        m_minMin.m_y <= rhs.m_minMin.m_y && m_maxMax.m_y >= rhs.m_maxMax.m_y;

    cornerlessOverlap |=
        rhs.m_minMin.m_x >= m_minMin.m_x && rhs.m_maxMax.m_x <= m_maxMax.m_x &&
        rhs.m_minMin.m_y <= m_minMin.m_y && rhs.m_maxMax.m_y >= m_maxMax.m_y;

    return cornerlessOverlap ||
        inRegion( rhs.m_minMin ) || inRegion( rhs.m_maxMax ) ||
        inRegion( XYPoint<CoordType>( rhs.m_minMin.m_x, rhs.m_maxMax.m_y ) ) ||
        inRegion( XYPoint<CoordType>( rhs.m_maxMax.m_x, rhs.m_minMin.m_y ) ) ||
        rhs.inRegion( m_minMin ) || rhs.inRegion( m_maxMax ) ||
        rhs.inRegion( XYPoint<CoordType>( m_minMin.m_x, m_maxMax.m_y ) ) ||
        rhs.inRegion( XYPoint<CoordType>( m_maxMax.m_x, m_minMin.m_y ) );
}

template<typename CoordType>
std::ostream &operator<<( std::ostream &stream, const XYPoint<CoordType> &val )
{
    stream << "(" << val.m_x << ", " << val.m_y << ")";

    return stream;
}

template<typename CoordType>
std::ostream &operator<<( std::ostream &stream, const RectangularRegion<CoordType> &val )
{
    stream << "<" << val.m_minMin << ", " << val.m_maxMax << ">";

    return stream;
}


template<typename CoordType, typename ValueType>
QuadTree<CoordType, ValueType>::QuadTree( size_t depth, CoordType xMin, CoordType xMax, CoordType yMin, CoordType yMax )
{
    CoordType width = (xMax - xMin) / 2.0;
    CoordType height = (yMax - yMin) / 2.0;
    CoordType xMid = xMin + width;
    CoordType yMid = yMin + height;
    
    m_splitStruct = SplitStruct( xMid, yMid, width, height, (int) depth );
}

template<typename CoordType, typename ValueType>
void QuadTree<CoordType, ValueType>::add( CoordType x, CoordType y, const ValueType &val )
{
    m_container.add( m_splitStruct, x, y, val );
}

template<typename CoordType, typename ValueType>
void QuadTree<CoordType, ValueType>::visitRegion(
    const RectangularRegion<CoordType> &bounds,
    boost::function<void( CoordType x, CoordType y, const ValueType & )> fn )
{
    m_container.visitRegion( m_splitStruct, bounds, fn );
}

// Implementation in router.cpp - move this declaration somewhere more useful
double distBetween( double lat1, double lon1, double lat2, double lon2 );

template<typename CoordType, typename ValueType>
class ClosestPointSearchFunctor
{
public:
    typedef XYPoint<CoordType> point_t;

private:
    bool m_found;
    point_t m_refPoint, m_closest;
    double m_closestDist;

public:
    ClosestPointSearchFunctor( const point_t &refPoint ) : m_found ( false ), m_refPoint( refPoint ),
        m_closestDist( std::numeric_limits<CoordType>::quiet_NaN() )
    {
    }

    bool found() { return m_found; }
    point_t closestPoint() { return m_closest; }

    CoordType distBetween( const point_t &first, const point_t &second )
    {
        return ::distBetween( first.m_x, first.m_y, second.m_x, second.m_y );
    }

    void operator()( CoordType x, CoordType y, const ValueType & )
    {
        point_t newCoord( x, y );
        CoordType dist = distBetween( m_refPoint, newCoord );

        if ( m_found || dist < m_closestDist )
        {
            m_closest = newCoord;
            m_closestDist = dist;
            m_found = true;
        }
    }
};

template<typename CoordType, typename ValueType>
XYPoint<CoordType> QuadTree<CoordType, ValueType>::closestPoint( const XYPoint<CoordType> &point )
{
    // Somewhat crap iterative algo - but should be pretty efficient under most conditions
    CoordType surveyWidth  = m_splitStruct.m_width / pow( 2.0, m_splitStruct.m_depthIter );
    CoordType surveyHeight = m_splitStruct.m_height / pow( 2.0, m_splitStruct.m_depthIter );

    ClosestPointSearchFunctor<CoordType, ValueType> f( point );

    do
    {
        RectangularRegion<CoordType> searchBounds(
            XYPoint<CoordType>( point.m_x - surveyWidth, point.m_y - surveyHeight ),
            XYPoint<CoordType>( point.m_x + surveyWidth, point.m_y + surveyHeight ) );

        std::cout << "Searching within bounds: " << searchBounds << std::endl;

        visitRegion( searchBounds, f );

        if ( f.found() )
        {
            return f.closestPoint();
        }

        surveyWidth  *= 2.0;
        surveyHeight *= 2.0;
    }
    while ( surveyWidth < 2.0*m_splitStruct.m_width || surveyHeight < 2.0*m_splitStruct.m_height );

    throw std::runtime_error( "No points found" );
}

template<typename CoordType, typename ValueType>
QuadTree<CoordType, ValueType>::SplitStruct::SplitStruct(
    CoordType xMid,
    CoordType yMid,
    CoordType width,
    CoordType height,
    size_t depthIter ) :
    m_xMid( xMid ), m_yMid( yMid ), m_width( width ), m_height( height ), m_depthIter( depthIter )
{
}

template<typename CoordType, typename ValueType>
typename QuadTree<CoordType, ValueType>::SplitStruct::splitQuad_t
QuadTree<CoordType, ValueType>::SplitStruct::whichQuad( CoordType x, CoordType y ) const
{
    size_t theQuad = 0;

    if ( x > m_xMid ) theQuad += 1;
    if ( y > m_yMid ) theQuad += 2;
    
    return (splitQuad_t) theQuad;
}

template<typename CoordType, typename ValueType>
typename QuadTree<CoordType, ValueType>::SplitStruct
QuadTree<CoordType, ValueType>::SplitStruct::executeSplit( splitQuad_t quad ) const
{
    SplitStruct newS( *this );
    newS.m_width  /= 2.0;
    newS.m_height /= 2.0;
    newS.m_depthIter -= 1;
    
    if ( newS.m_depthIter < 0 )
    {
        throw std::exception();
    }
    
    if ( quad & 1 ) newS.m_xMid += newS.m_width;
    else newS.m_xMid -= newS.m_width;
    
    if ( quad & 2 ) newS.m_yMid += newS.m_height;
    else newS.m_yMid -= newS.m_height;
    
    return newS;
}

template<typename CoordType, typename ValueType>
RectangularRegion<CoordType> QuadTree<CoordType, ValueType>::SplitStruct::rectFor( splitQuad_t quad ) const
{
    double minX, maxX, minY, maxY;
    
    if ( quad & 1 ) 
    {
        minX = m_xMid;
        maxX = m_xMid + m_width; 
    }
    else
    {
        minX = m_xMid - m_width;
        maxX = m_xMid;
    }

    if ( quad & 2 )
    {
        minY = m_yMid;
        maxY = m_yMid + m_height;
    }
    else
    {
        minY = m_yMid - m_height;
        maxY = m_yMid;
    }

    return RectangularRegion<CoordType>( minX, minY, maxX, maxY );
}

template<typename CoordType, typename ValueType>
/*virtual*/ void QuadTree<CoordType, ValueType>::TMVecContainer::add(
    const SplitStruct &s,
    CoordType x,
    CoordType y,
    const ValueType &val )
{
    RectangularRegion<CoordType> debugRegion( s.m_xMid - s.m_width, s.m_yMid - s.m_height, s.m_xMid + s.m_height, s.m_yMid + s.m_height );

    if ( !debugRegion.inRegion( XYPoint<CoordType>( x, y ) ) )
    {
        std::cout << "Point placed in wrong quadrant" << std::endl;
    }
    m_values.push_back( boost::make_tuple( x, y, val ) );
}

    
template<typename CoordType, typename ValueType>
/*virtual*/ void QuadTree<CoordType, ValueType>::TMQuadContainer::add(
    const SplitStruct &s,
    CoordType x,
    CoordType y,
    const ValueType &val )
{
    size_t thisDepth = s.getDepth();   
    if ( m_quadrants.empty() )
    {
        for ( size_t i = 0; i < 4; i++ )
        {
            if ( thisDepth == 0 )
            {
                m_quadrants.push_back( new TMVecContainer() );
            }
            else
            {
                m_quadrants.push_back( new TMQuadContainer() );
            }
        }
    }

    typename SplitStruct::splitQuad_t theQuad = s.whichQuad( x, y );
    m_quadrants[theQuad]->add( s.executeSplit( theQuad ), x, y, val );
}

template<typename CoordType, typename ValueType>
/*virtual*/ void QuadTree<CoordType, ValueType>::TMVecContainer::visitRegion(
    const SplitStruct &s,
    const RectangularRegion<CoordType> &bounds,
    visitFn_t fn )
{
    //std::cout << "Visiting vec container with: " << m_values.size() << std::endl;
    typedef typename QuadTree<CoordType, ValueType>::TMVecContainer::coordEl_t coordEl_t;

    BOOST_FOREACH( const coordEl_t &v, m_values )
    {
        CoordType x = v.template get<0>();
        CoordType y = v.template get<1>();
        const ValueType &val = v.template get<2>();
        
        if ( bounds.inRegion( XYPoint<CoordType>( x, y ) ) )
        {
            fn( x, y, val );
        }
    }
}


template<typename CoordType, typename ValueType>
/*virtual*/ void QuadTree<CoordType, ValueType>::TMQuadContainer::visitRegion(
    const SplitStruct &s,
    const RectangularRegion<CoordType> &bounds,
    visitFn_t fn )
{
    //std::cout << "Visiting a quad container" << std::endl;
    if ( !m_quadrants.empty() )
    {
        for ( int i = 0; i < 4; i++ )
        {
            typename SplitStruct::splitQuad_t theQuad = (typename SplitStruct::splitQuad_t) i;

            RectangularRegion<CoordType> r = s.rectFor( theQuad );

            if ( r.overlaps( bounds ) )
            {
                m_quadrants[i]->visitRegion( s.executeSplit( theQuad ), bounds, fn );
            }
        }
    }
}

template<typename CoordType, typename ValueType>
/*virtual*/ QuadTree<CoordType, ValueType>::TMQuadContainer::~TMQuadContainer()
{
    BOOST_FOREACH( const TMContBase *el, m_quadrants )
    {
        delete el;
    }
}
