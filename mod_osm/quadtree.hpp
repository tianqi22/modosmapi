#ifndef QUADTREE_H
#define QUADTREE_H

#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <vector>

template<typename CoordType>
struct XYPoint
{
    CoordType m_x, m_y;
    
    XYPoint( CoordType x, CoordType y ) : m_x( x ), m_y( y )
    {
    }
};

template<typename CoordType>
struct RectangularRegion
{
    XYPoint<CoordType> m_minMin, m_maxMax;

    bool inRegion( const XYPoint<CoordType> &point ) const;
    bool overlaps( const RectangularRegion<CoordType> &rhs ) const;
    
    RectangularRegion( CoordType min_x, CoordType min_y, CoordType max_x, CoordType max_y ) :
        m_minMin( min_x, min_y ), m_maxMax( max_x, max_y )
    {
    }
};

template<typename CoordType>
std::ostream &operator<<( std::ostream &stream, const XYPoint<CoordType> &val );

template<typename CoordType>
std::ostream &operator<<( std::ostream &stream, const RectangularRegion<CoordType> &val );

template<typename CoordType, typename ValueType>
class QuadTree
{
public:
    typedef boost::function<void( CoordType x, CoordType y, const ValueType & )> visitFn_t;

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
        
    public:
        CoordType m_xMid, m_yMid;
        
        // Width/height of a single quadrant (1/2 the total width/height)
        CoordType m_width, m_height;
        size_t m_depthIter;
        
    public:
        SplitStruct() {}
        SplitStruct( CoordType xMid, CoordType yMid, CoordType width, CoordType height, size_t depthIter );
        splitQuad_t whichQuad( CoordType x, CoordType y ) const;
        SplitStruct executeSplit( enum splitQuad_t ) const;
        RectangularRegion<CoordType> rectFor( enum splitQuad_t ) const;
        size_t getDepth() const { return m_depthIter; }
    };

    class TMContBase
    {
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val ) = 0;
        virtual void visitRegion(
            const SplitStruct &s,
            const RectangularRegion<CoordType> &bounds,
            visitFn_t fn ) = 0;
        
        // TODO: If we want this, may need to change base container away from vector
        //virtual void erase( const SplitStruct &s, CoordType x, CoordType y ) = 0;
        virtual ~TMContBase() {}
    };
    
    class TMVecContainer : public TMContBase
    {
    public:
        typedef boost::tuple<CoordType, CoordType, ValueType> coordEl_t;

    private:
        std::vector<coordEl_t> m_values;

    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
        virtual void visitRegion(
            const SplitStruct &s,
            const RectangularRegion<CoordType> &bounds,
            visitFn_t fn );
    };
    
    class TMQuadContainer : public TMContBase
    {
        std::vector<TMContBase *> m_quadrants;
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
        virtual void visitRegion(
            const SplitStruct &s,
            const RectangularRegion<CoordType> &bounds,
            visitFn_t fn );
        virtual ~TMQuadContainer();
    };

    SplitStruct     m_splitStruct;
    TMQuadContainer m_container;

public:
    QuadTree( size_t depth, CoordType xMin, CoordType xMax, CoordType yMin, CoordType yMax );
    void add( CoordType x, CoordType y, const ValueType &val );
    void visitRegion( const RectangularRegion<CoordType> &bounds, visitFn_t fn );
    XYPoint<CoordType> closestPoint( const XYPoint<CoordType> &point );
};

#include "quadtree.ipp"

#endif // QUADTREE_H

