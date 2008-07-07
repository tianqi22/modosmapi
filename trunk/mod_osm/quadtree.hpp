#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <vector>

template<typename CoordType, typename ValueType>
class QuadTree
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
        virtual void visitRegion(
            const SplitStruct &s,
            CoordType xMin, CoordType xMax,
            CoordType yMin, CoordType yMax,
            boost::function<void( const ValueType & )> fn ) = 0;
        
        // TODO: If we want this, may need to change base container away from vector
        //virtual void erase( const SplitStruct &s, CoordType x, CoordType y ) = 0;
        virtual ~TMContBase() {}
    };
    
    class TMVecContainer : TMContBase
    {
        typedef boost::tuple<CoordType, CoordType, ValueType> CoordEl_t;
        std::vector<CoordEl_t> m_values;
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
        virtual void visitRegion(
            const SplitStruct &s,
            CoordType xMin, CoordType xMax,
            CoordType yMin, CoordType yMax,
            boost::function<void( const ValueType & )> fn );
    };
    
    class TMQuadContainer : TMContBase
    {
        std::vector<TMContBase *> m_quadrants;
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
        virtual void visitRegion(
            const SplitStruct &s,
            CoordType xMin, CoordType xMax,
            CoordType yMin, CoordType yMax,
            boost::function<void( const ValueType & )> fn );
        virtual ~TMQuadContainer();
    };

    SplitStruct     m_splitStruct;
    TMQuadContainer m_container;
public:
    QuadTree( size_t depth, CoordType xMin, CoordType xMax, CoordType yMin, CoordType yMax );
    void add( CoordType x, CoordType y, const ValueType &val );
    void visitRegion(
        CoordType xMin, CoordType xMax,
        CoordType yMin, CoordType yMax,
        boost::function<void( const ValueType & )> fn );
};

