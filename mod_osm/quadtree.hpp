#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

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
        virtual ~TMContBase() {}
    };
    
    class TMVecContainer : TMContBase
    {
        std::vector<boost::tuple<CoordType, CoordType, ValueType> > m_values;
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
    };
    
    class TMQuadContainer : TMContBase
    {
        std::vector<TMContBase *> m_quadrants;
    public:
        virtual void add( const SplitStruct &s, CoordType x, CoordType y, const ValueType &val );
        virtual ~TMQuadContainer();
    };

    SplitStruct     m_splitStruct;
    TMQuadContainer m_container;
public:
    TreeMap( size_t depth, CoordType left, CoordType right, CoordType top, CoordType bottom );
    void add( CoordType x, CoordType y, const ValueType &val );
};

