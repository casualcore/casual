/*
 * database.hpp
 *
 *  Created on: 2 nov 2012
 *      Author: hbergk
 */

/*
    sqlite3 database class
    author: Jan Nikolas Jansen
    date: 2012-04-13

    written in c++11, tested with g++4.6.3 using -std=c++0x
    link with -lsqlite3
*/

#ifndef database_HPP
#define database_HPP

// sqlite3 C API header
#include <sqlite3.h>

// STL containers used
#include <string>
#include <map>
#include <vector>
#include <sstream>

/**
 * sqlite3 database class.
 * @author nijansen
 *
 * Enhanced C++11 version of the sqlite3 database management class published
 * in my blog earlier. Supports escaping most values to protect against SQL
 * injections by making use of variadic templates. Enables foreign key support
 * by default.
 *
 * Since this class is heavy in templates and thus cannot be neatly divided into
 * a header and source file anyway, it is published as header only.
 */
class database {
public:
    /**
     * Connects to a database.
     *
     * If the database file does not exist, it will be created if possible and
     * then opened. Foreign key support will be enabled. If everything was
     * successful, the class will boolcast to true. If something went wrong, the
     * error function provides more details.
     *
     * @param[in] filename Location of the database file.
     */
    explicit database(const std::string &filename)
        : connected_(false)
    {
        if(sqlite3_open(filename.data(), &database_) == SQLITE_OK
            && sql("PRAGMA foreign_keys = ON;"))
        {
            connected_ = true;
        }
    }

    /**
     * Closes the connection to the database.
     */
    virtual ~database() { sqlite3_close(database_); }

    /**
     * Checks for correct initialization.
     *
     * @return true, if successful. Check the error function otherwise.
     */
    explicit operator bool() const { return connected_; }

    /** Definition of a database table row. **/
    typedef std::map<std::string, std::vector<std::string>> Row;

    /**
     * SQL Query, non-responsive version.
     *
     * Use this version if you execute statements that do not return a result
     * (like INSERT, UPDATE, etc.). You can use escaped parameters in your query
     * by writing a ? in the string instead of the desired value and appending
     * the values after the string in chronologic order.
     *
     * @param[in] query SQL statement, possibly with placeholders.
     * @param[in] args Argument list of values that need to be escaped.
     * @return true, if successful. Check the error function otherwise.
     */
    template <typename... Args> bool sql(const std::string &query,
        const Args &... args)
    {
        sql_statement call;
        if(sqlite3_prepare_v2(database_, query.data(), -1, &call.statement,
            nullptr) != SQLITE_OK)
        {
            return false;
        }
        if(!bind(call.statement, 1, args...)) {
            return false;
        }
        return sqlite3_step(call.statement) == SQLITE_DONE;
    }

    /**
     * SQL Query, responsive version.
     *
     * Use this version if you execute statements that do return a result (like
     * SELECT). You can use escaped parameters in your query by writing a ? in
     * the string instead of the desired value and appending the values after
     * the string in chronologic order.
     *
     * @param[in] query SQL statement, possibly with placeholders.
     * @param[in] rows Container for result that will be modified on success.
     * @param[in] args Argument list of values that need to be escaped.
     * @return true, if successful. Check the error function otherwise.
     */
    template <typename... Args> bool sql(const std::string &query,
        std::vector<Row> &rows, const Args &... args)
    {
        sql_statement call;
        if(sqlite3_prepare_v2(database_, query.data(), -1, &call.statement,
            nullptr) != SQLITE_OK)
        {
            return false;
        }
        if(!bind(call.statement, 1, args...)) {
            return false;
        }
        return get_rows(call.statement, rows);
    }

    /**
     * Is safe to call even when no error has occured yet.
     *
     * @return Last error message.
     */
    const std::string error() const {
        return sqlite3_errmsg(database_);
    }
private:
    /** sqlite3 statement wrapper for finalization **/
    struct sql_statement {
        sqlite3_stmt *statement;
        sql_statement() : statement(nullptr) {}
        ~sql_statement() { sqlite3_finalize(statement); }
    };

    /** bind dummy function for empty argument lists **/
    static bool bind(sqlite3_stmt *, const int) { return true; }

    /** bind delegator function that will call a specialized bind_struct **/
    template <typename T, typename... Args>
    static bool bind(sqlite3_stmt *statement,
        const int current, const T &first, const Args &... args)
    {
        return bind_struct<T, Args...>::f(statement, current,
            first, args...);
    }

    /** most general bind_struct that relies on implicit string conversion **/
    template <typename T, typename... Args>
    struct bind_struct {
        static bool f(sqlite3_stmt *statement, int current,
            const T &first, const Args &... args)
        {
            std::stringstream ss;
            ss << first;
            if(sqlite3_bind_text(statement, current,
                ss.str().data(), ss.str().length(),
                SQLITE_TRANSIENT) != SQLITE_OK)
            {
                return false;
            }
            return bind(statement, current+1, args...);
        }
    };

    /** bind_struct for double values **/
    template <typename... Args>
    struct bind_struct<double, Args...> {
        static bool f(sqlite3_stmt *statement, int current,
            double first, const Args &... args)
        {
            if(sqlite3_bind_double(statement, current, first)
                != SQLITE_OK)
            {
                return false;
            }
            return bind(statement, current+1, args...);
        }
    };

    /** bind_struct for int values **/
    template <typename... Args>
    struct bind_struct<int, Args...> {
        static bool f(sqlite3_stmt *statement, int current,
            int first, const Args &... args)
        {
            if(sqlite3_bind_int(statement, current, first)
                != SQLITE_OK)
            {
                return false;
            }
            return bind(statement, current+1, args...);
        }
    };

    /** bind_struct for byte arrays **/
    template <typename... Args>
    struct bind_struct<std::vector<char>, Args...> {
        static bool f(sqlite3_stmt *statement, int current,
            const std::vector<char> &first, const Args &... args)
        {
            if(sqlite3_bind_blob(statement, current,
                &first[0], first.size(),
                SQLITE_TRANSIENT) != SQLITE_OK)
            {
                return false;
            }
            return bind(statement, current+1, args...);
        }
    };

    /** retrieves rows from a prepared and bound statement **/
    bool get_rows(sqlite3_stmt *statement, std::vector<Row> &rows) {
        int retval = 0;
        do {
            retval = sqlite3_step(statement);
            if(retval != SQLITE_ROW && retval != SQLITE_DONE) {
                return false;
            }
            if(retval == SQLITE_ROW) {
                Row row;
                const auto ncols = sqlite3_column_count(statement);
                for(int i = 0; i < ncols; i++) {
                    const auto colname = sqlite3_column_name(statement, i);
                    const auto colbytes = sqlite3_column_text(statement, i);
                    const auto nbytes = sqlite3_column_bytes(statement, i);
                    if(colbytes) {
                        std::string colcontent;
                        colcontent.assign((char *)&colbytes[0], nbytes);
                        row[colname] = std::vector<std::string>(1, colcontent);
                    }
                    else {
                        row[colname] = std::vector<std::string>();
                    }
                }
                rows.push_back(row);
            }
        } while(retval != SQLITE_DONE);
        return true;
    }

    /** true if initialized **/
    bool connected_;
    /** sqlite3 database handle **/
    sqlite3 *database_;
};
#endif // database_HPP
