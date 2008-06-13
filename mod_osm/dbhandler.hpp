#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_array.hpp>

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

        template<typename T> void executeBulkInsert( std::string query, const std::vector<T> &rows );

    private:
        void bindArg( MYSQL_BIND &args, const std::string &value );
        void bindArg( MYSQL_BIND &args, const int &value );
        void bindArg( MYSQL_BIND &args, const boost::uint64_t &value );
        void bindArg( MYSQL_BIND &args, const boost::int64_t &value );
        void bindArg( MYSQL_BIND &args, const double &value );
        void bindArg( MYSQL_BIND &args, const bool &value );
        void bindArg( MYSQL_BIND &args, const boost::posix_time::ptime &datetime );

        template<typename B, typename T1, typename T2>
        void bindArgsRec( B &args, const boost::tuples::cons<T1, T2> &row, int offset );

        template<typename B, typename T>
        void bindArgsRec( B &args, const boost::tuples::cons<T, boost::tuples::null_type> &row, int offset );

    private:
        MYSQL m_dbconn;
        MYSQL_RES *m_res;
        MYSQL_ROW m_row;

        static bool created;

        std::vector<boost::shared_array<char> > m_bulkInsertBuf;

    };
}


#include "dbhandler.ipp"

