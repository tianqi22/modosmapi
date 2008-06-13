#include <string>

#include <boost/cstdint.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <mysql/mysql.h>

namespace modosmapi
{

    class SqlException : public std::exception
    {
        std::string m_message;

    public:
        SqlException( const std::string &message ) : m_message( message ) { }
        virtual ~SqlException() throw () { }
        const std::string &getMessage() const { return m_message; }
    };

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

        void execute_noresult( std::string query );
        void execute( std::string query );
        bool next();

        template<typename T> T getField( size_t index );
        template<typename T>
        void readRow( boost::tuples::cons<T, boost::tuples::null_type> &t, size_t index=0 );


        void bindArg( MYSQL_BIND *args, const std::string &value, int offset );
        void bindArg( MYSQL_BIND *args, boost::uint64_t value, int offset );
        void bindArg( MYSQL_BIND *args, boost::int64_t value, int offset );
        void bindArg( MYSQL_BIND *args, double value, int offset );
        void bindArg( MYSQL_BIND *args, bool value, int offset );
        void bindArg( MYSQL_BIND *args, const boost::posix_time::ptime &datetime, int offset );

        // e.g. INSERT INTO blah VALUES ( ?, ?, ?, ? )
        template<typename T> void execute_bulk_insert( std::string query, const std::vector<T> &rows );

        template<typename T1, typename T2>
        void bindArgsRec( MYSQL_BIND *args, boost::tuples::cons<T1, T2> &row, int offset );

        template<typename T>
        void bindArgsRec( MYSQL_BIND *args, boost::tuples::cons<T, boost::tuples::null_type> &row, int offset );

        template<typename T1, typename T2>
        void readRow( boost::tuples::cons<T1, T2> &t, size_t index=0 );


    private:
        MYSQL m_dbconn;
        MYSQL_RES *m_res;
        MYSQL_ROW m_row;

        static bool created;

    };
}


#include "dbhandler.ipp"

