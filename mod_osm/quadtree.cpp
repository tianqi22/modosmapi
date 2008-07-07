#include <boost/tuple/tuple.hpp>

#include <vector>



template<typename CoordType, typename ValueType>
class TreeMap
{
    class SplitStruct
    {
    public:
        enum splitQuad_t
        {
            QUAD_TL = 0,
            QUAD_TR = 1,
            QUAD_BL = 2,
            QUAD_BR = 3
        };
        
    private:
        CoordType m_xMid, m_yMid;
        
        // Width/height of a single quadrant (1/2 the total width/height)
        CoordType m_width, m_height;
        int m_depthIter;
        
    public:
        SplitStruct( CoordType xMid, CoordType yMid, CoordType width, CoordType height, int depthIter );
        splitQuad_t whichQuad( CoordType x, CoordType y ) const;
        SplitStruct executeSplit( enum splitQuad_t ) const ;
    };

    class TMContBase
    {
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val ) = 0;
    };
    
    class TMVecContainer : TMContBase
    {
        std::vector<boost::tuple<CoordType, CoordType, ValueType> > m_values;
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
    };
    
    class TMQuadContainer : TMContBase
    {
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
    };

    SplitStruct     m_splitStruct;
    TMQuadContainer m_container;
public:
    TreeMap( size_t depth, CoordType left, CoordType right, CoordType top, CoordType bottom );
    void add( CoordType x, CoordType y, const ValueType &val );
};

template<typename CoordType, typename ValueType>
TreeMap<CoordType, ValueType>::TreeMap( size_t depth, CoordType left, CoordType right, CoordType top, CoordType bottom )
{
    CoordType width = (right - left) / 2.0;
    CoordType height = (top - bottom) / 2.0;
    CoordType xMid = left + width;
    CoordType yMid = bottom+ height;
    
    m_splitStruct = SplitStruct( xMid, yMid, width, height, depth );
}

template<typename CoordType, typename ValueType>
void TreeMap<CoordType, ValueType>::add( CoordType x, CoordType y, const ValueType &val )
{
    // TODO
}

template<typename CoordType, typename ValueType>
TreeMap<CoordType, ValueType>::SplitStruct::SplitStruct(
    CoordType xMid,
    CoordType yMid,
    CoordType width,
    CoordType height,
    int depthIter ) :
    m_xMid( xMid ), m_yMid( yMid ), m_width( width ), m_height( height ), m_depthIter( depthIter )
{
}

template<typename CoordType, typename ValueType>
typename TreeMap<CoordType, ValueType>::SplitStruct::splitQuad_t
TreeMap<CoordType, ValueType>::SplitStruct::whichQuad( CoordType x, CoordType y ) const
{
    splitQuad_t theQuad = 0;
    if ( x > m_xMid ) theQuad += 1;
    if ( y > m_yMid ) theQuad += 2;
    
    return theQuad;
}

template<typename CoordType, typename ValueType>
typename TreeMap<CoordType, ValueType>::SplitStruct
TreeMap<CoordType, ValueType>::SplitStruct::executeSplit( splitQuad_t quad ) const
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
/*virtual*/ void TreeMap<CoordType, ValueType>::TMVecContainer::add(
    const SplitStruct &s,
    CoordType x,
    CoordType y,
    const ValueType &val )
{
    m_values.push_back( boost::make_tuple( x, y, val ) );
}

template<typename CoordType, typename ValueType>
/*virtual*/ void TreeMap<CoordType, ValueType>::TMQuadContainer::add(
    const SplitStruct &s,
    CoordType x,
    CoordType y,
    const ValueType &val )
{
}

