#include "quadtree.hpp"

#include <boost/foreach.hpp>

template<typename CoordType, typename ValueType>
QuadTree<CoordType, ValueType>::QuadTree( size_t depth, CoordType left, CoordType right, CoordType top, CoordType bottom )
{
    CoordType width = (right - left) / 2.0;
    CoordType height = (top - bottom) / 2.0;
    CoordType xMid = left + width;
    CoordType yMid = bottom+ height;
    
    m_splitStruct = SplitStruct( xMid, yMid, width, height, depth );
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
    boost::function<void( const ValueType & )> fn )
{
    m_container.visitRegion( m_splitStruct, xMin, xMax, yMin, yMax, fn );
}

template<typename CoordType, typename ValueType>
QuadTree<CoordType, ValueType>::SplitStruct::SplitStruct(
    CoordType xMid,
    CoordType yMid,
    CoordType width,
    CoordType height,
    int depthIter ) :
    m_xMid( xMid ), m_yMid( yMid ), m_width( width ), m_height( height ), m_depthIter( depthIter )
{
}

template<typename CoordType, typename ValueType>
typename QuadTree<CoordType, ValueType>::SplitStruct::splitQuad_t
QuadTree<CoordType, ValueType>::SplitStruct::whichQuad( CoordType x, CoordType y ) const
{
    splitQuad_t theQuad = 0;
    if ( x > m_xMid ) theQuad += 1;
    if ( y > m_yMid ) theQuad += 2;
    
    return theQuad;
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
/*virtual*/ void QuadTree<CoordType, ValueType>::TMVecContainer::visitRegion(
    const SplitStruct &s,
    CoordType xMin, CoordType xMax,
    CoordType yMin, CoordType yMax,
    boost::function<void( const ValueType & )> fn )
{
    typedef typename QuadTree<CoordType, ValueType>::TMVecContainer::coordEl_t coordEl_t;

    BOOST_FOREACH( const coordEl_t &v, m_values )
    {
//          CoordType x = v.get<0>();
//          CoordType y = v.get<1>();
//          const ValueType &val = v.get<2>();
        
//          if ( x >= xMin && x =< xMax && y >= yMin && y < yMax )
//          {
//              fn( val );
//          }
    }
}
    
template<typename CoordType, typename ValueType>
/*virtual*/ void QuadTree<CoordType, ValueType>::TMQuadContainer::add(
    const SplitStruct &s,
    CoordType x,
    CoordType y,
    const ValueType &val )
{
    size_t thisDepth = s.m_depth;   
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
    m_quadrants.add( s.executeSplit( theQuad ), x, y, val );
}

template<typename CoordType, typename ValueType>
/*virtual*/ void QuadTree<CoordType, ValueType>::TMQuadContainer::visitRegion(
    const SplitStruct &s,
    CoordType xMin, CoordType xMax,
    CoordType yMin, CoordType yMax,
    boost::function<void( const ValueType & )> fn )
{
    // TODO: Get coords of each quad, then bounds check on each
}

template<typename CoordType, typename ValueType>
/*virtual*/ QuadTree<CoordType, ValueType>::TMQuadContainer::~TMQuadContainer()
{
    BOOST_FOREACH( const TMContBase *el, m_quadrants )
    {
        delete el;
    }
}
