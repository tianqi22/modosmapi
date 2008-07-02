#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <mysql/mysql.h>

namespace modosmapi
{

    class SqlException : public std::exception
    {
        std::string m_message;

    public:
        SqlException( const std::string &message ) : m_message( message ) { }
        virtual ~SqlException() throw () {}
        const std::string &getMessage() const { return m_message; }
    };

    class BindArgDataHolder
    {
    private:
        MYSQL *m_dbConn;
        MYSQL_STMT *m_ps;
        boost::scoped_array<MYSQL_BIND> m_args;
        std::vector<boost::shared_array<char> > m_argMem;

    public:
        BindArgDataHolder( MYSQL *dbConn, MYSQL_STMT *ps );

        template<typename T> void bindParams( const T &row );
        template<typename T> void bindResults( const T &row );
        template<typename T> void setArgs( const T &row );
        template<typename T> void getArgs( T &row );

    private:

        // TODO: Perhaps pass a ref to the MYSQL_BIND and a ref to m_argMem and
        //       remove the index (no need)
        class ArgBinder
        {
            struct ArgParams_t
            {
                my_bool is_null;
                unsigned long length;
                my_bool error;
            };

            MYSQL *m_dbConn;
            std::vector<boost::shared_array<char> > &m_argMem;
        public:
            ArgBinder( MYSQL *dbConn, std::vector<boost::shared_array<char> > &argMem ) : m_dbConn( dbConn ), m_argMem( argMem )
            {
            }
            void setMem( MYSQL_BIND &arg, enum enum_field_types type );
            void operator()( MYSQL_BIND &arg, const std::string &value );
            void operator()( MYSQL_BIND &arg, const int &value );
            void operator()( MYSQL_BIND &arg, const boost::uint64_t &value );
            void operator()( MYSQL_BIND &arg, const boost::int64_t &value );
            void operator()( MYSQL_BIND &arg, const double &value );
            void operator()( MYSQL_BIND &arg, const bool &value );
            void operator()( MYSQL_BIND &arg, const boost::posix_time::ptime &datetime );
        };

        class ArgSetter
        {
        public:
            void operator()( MYSQL_BIND &arg, const std::string &value );
            void operator()( MYSQL_BIND &arg, const int &value );
            void operator()( MYSQL_BIND &arg, const boost::uint64_t &value );
            void operator()( MYSQL_BIND &arg, const boost::int64_t &value );
            void operator()( MYSQL_BIND &arg, const double &value );
            void operator()( MYSQL_BIND &arg, const bool &value );
            void operator()( MYSQL_BIND &arg, const boost::posix_time::ptime &datetime );
        };

        class ArgGetter
        {
        public:
            void operator()( MYSQL_BIND &arg, std::string &value );
            void operator()( MYSQL_BIND &arg, int &value );
            void operator()( MYSQL_BIND &arg, boost::uint64_t &value );
            void operator()( MYSQL_BIND &arg, boost::int64_t &value );
            void operator()( MYSQL_BIND &arg, double &value );
            void operator()( MYSQL_BIND &arg, bool &value );
            void operator()( MYSQL_BIND &arg, boost::posix_time::ptime &datetime );
        };

    private:
        template<typename F, typename T1, typename T2>
        void applyRec( F &fn, const boost::tuples::cons<T1, T2> &temp, size_t index );
        template<typename F, typename T>
        void applyRec( F &fn, const boost::tuples::cons<T, boost::tuples::null_type> &temp, size_t index );

        template<typename F, typename T1, typename T2>
        void applyRec( F &fn, boost::tuples::cons<T1, T2> &temp, size_t index );
        template<typename F, typename T>
        void applyRec( F &fn, boost::tuples::cons<T, boost::tuples::null_type> &temp, size_t index );        

    };
    
    template<typename T>
    void BindArgDataHolder::bindParams( const T &row )
    {
        m_args.reset( new MYSQL_BIND[boost::tuples::length<T>::value] );
        
        ArgBinder b( m_dbConn, m_argMem );
        applyRec( b, row, 0 );

        if ( mysql_stmt_bind_param( m_ps, m_args.get() ) )
        {
            std::cerr << "Param binding failed: " << mysql_error( m_dbConn ) << std::endl;
            throw SqlException( std::string( "Param binding failed: " ) );//+ mysql_error( &m_dbconn ) );
        } 
    }

    template<typename T>
    void BindArgDataHolder::bindResults( const T &row )
    {
        m_args.reset( new MYSQL_BIND[boost::tuples::length<T>::value] );
        
        ArgBinder b( m_dbConn, m_argMem );
        applyRec( b, row, 0 );

        if ( mysql_stmt_bind_result( m_ps, m_args.get() ) )
        {       
            throw SqlException( std::string( "Result binding failed: " ) );//+ mysql_error( &m_dbconn ) );
        } 
    }

    
    template<typename T>
    void BindArgDataHolder::setArgs( const T &row )
    {
        ArgSetter s;
        applyRec( s, row, 0 );
    }

    template<typename T>
    void BindArgDataHolder::getArgs( T &row )
    {
        ArgGetter g;
        applyRec( g, row, 0 );
    }

    template<typename F, typename T1, typename T2>
    void BindArgDataHolder::applyRec( F &fn, const boost::tuples::cons<T1, T2> &temp, size_t index )
    {
        fn( m_args[index], temp.head );
        applyRec( fn, temp.tail, index + 1 );
    }
    template<typename F, typename T>
    void BindArgDataHolder::applyRec( F &fn, const boost::tuples::cons<T, boost::tuples::null_type> &temp, size_t index )
    {
        fn( m_args[index], temp.head );
    }

    template<typename F, typename T1, typename T2>
    void BindArgDataHolder::applyRec( F &fn, boost::tuples::cons<T1, T2> &temp, size_t index )
    {
        fn( m_args[index], temp.head );
        applyRec( fn, temp.tail, index + 1 );
    }
    template<typename F, typename T>
    void BindArgDataHolder::applyRec( F &fn, boost::tuples::cons<T, boost::tuples::null_type> &temp, size_t index )
    {
        fn( m_args[index], temp.head );
    }


    class DbConnection
    {
    public:
        DbConnection(
            const std::string &dbhost,
            const std::string &dbname,
            const std::string &dbuser,
            const std::string &dbpass );
        ~DbConnection();

        void cleanUp();

        void executeNoResult( std::string query );
        void execute( std::string query );
        bool next();

        template<typename T> T getField( size_t index );
        
        template<typename T>
        void readRow( boost::tuples::cons<T, boost::tuples::null_type> &t, size_t index=0 );

        template<typename T1, typename T2>
        void readRow( boost::tuples::cons<T1, T2> &t, size_t index=0 );

        template<typename T> void executeBulkInsert  ( std::string query, const std::vector<T> &rows );
        template<typename T> void executeBulkRetrieve( std::string query, boost::function<void( const T & )> fn );
        template<typename T> void executeBulkRetrieve( std::string query, std::vector<T> &result );

    private:
        MYSQL m_dbconn;
        MYSQL_RES *m_res;
        MYSQL_ROW m_row;

        static bool created;

        std::vector<boost::shared_array<char> > m_bulkInsertBuf;

    };
}


#include "dbhandler.ipp"

#endif // DBHANDLER_H
