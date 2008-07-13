
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
           rhs.inRegion( m_minMin ) || rhs.inRegion( m_maxMax );
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
    CoordType xMin, CoordType xMax,
    CoordType yMin, CoordType yMax,
    boost::function<void( CoordType x, CoordType y, const ValueType & )> fn )
{
    m_container.visitRegion( m_splitStruct, xMin, xMax, yMin, yMax, fn );
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
/*virtual*/ void QuadTree<CoordType, ValueType>::TMVecContainer::add(
    const SplitStruct &s,
    CoordType x,
    CoordType y,
    const ValueType &val )
{
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
    CoordType xMin, CoordType xMax,
    CoordType yMin, CoordType yMax,
    visitFn_t fn )
{
    std::cout << "Visiting vec container with: " << m_values.size() << std::endl;
    typedef typename QuadTree<CoordType, ValueType>::TMVecContainer::coordEl_t coordEl_t;

    BOOST_FOREACH( const coordEl_t &v, m_values )
    {
        CoordType x = v.template get<0>();
        CoordType y = v.template get<1>();
        const ValueType &val = v.template get<2>();
        
        if ( x >= xMin && x < xMax && y >= yMin && y < yMax )
        {
            fn( x, y, val );
        }
    }
}


template<typename CoordType, typename ValueType>
/*virtual*/ void QuadTree<CoordType, ValueType>::TMQuadContainer::visitRegion(
    const SplitStruct &s,
    CoordType xMin, CoordType xMax,
    CoordType yMin, CoordType yMax,
    visitFn_t fn )
{
    std::cout << "Visiting a quad container" << std::endl;
    if ( !m_quadrants.empty() )
    {
        for ( int i = 0; i < 4; i++ )
        {
            typename SplitStruct::splitQuad_t theQuad = (typename SplitStruct::splitQuad_t) i;

            m_quadrants[i]->visitRegion( s.executeSplit( theQuad ),
                xMin, xMax, yMin, yMax, fn );
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
