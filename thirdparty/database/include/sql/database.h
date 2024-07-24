//!
//! Copyright (c) 2013, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <sqlite3.h>

#include "casual/concepts.h"
#include "common/algorithm.h"
#include "common/string.h"
#include "common/compare.h"
#include "common/code/raise.h"
#include "common/code/serialize.h"


#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <filesystem>
#include <fstream>


#include <cstring>

namespace sql
{
   namespace database
   {
      namespace code
      {
         enum class sql : int 
         {
            ok = SQLITE_OK,
            row = SQLITE_ROW,
            done = SQLITE_DONE,
            cant_open = SQLITE_CANTOPEN,
         };

         static_assert( static_cast< int>( sql::ok) == 0, "sql::ok has to be 0"); 

         namespace detail
         {
            struct Category : std::error_category
            {
               inline const char* name() const noexcept override
               {
                  return "sql";
               }

               inline std::string message( int code) const override
               {
                  return sqlite3_errstr( code);
               }
            };
            inline static const Category& category()
            {
               static auto& category = casual::common::code::serialize::registration< Category>( 0xa5fb63c2e7f343c3a8b0147f3f0a667a_uuid);
               return category;
            }
         } // detail

         inline std::error_code make( int code)
         {
            return { code, detail::category()};
         }

         inline std::error_code make_error_code( code::sql code)
         {
            return { static_cast< int>( code), detail::category()};
         }

         template< typename H>
         [[noreturn]] void raise( std::error_code code, H&& handle)
         {
            throw std::system_error{ code, sqlite3_errmsg( handle.get())};
         }
      } // code

      namespace version
      {
         inline auto library()
         {
            return sqlite3_libversion_number();
         }
      } // version

   } // database
} // sql

namespace std
{
   template <>
   struct is_error_code_enum< sql::database::code::sql> : true_type {};
}

namespace sql
{
   namespace database
   {
      namespace memory
      {
         constexpr auto file = ":memory:";
      } // memory

      using duration_type = std::chrono::microseconds;


      inline auto parameter_bind( sqlite3_stmt* statement, int index, std::nullptr_t)
      {
         return code::make( sqlite3_bind_null( statement, index));
      }

      inline auto parameter_bind( sqlite3_stmt* statement, int index, const std::string& value)
      {
         return code::make( sqlite3_bind_text( statement, index, value.data(), value.size(), SQLITE_STATIC));
      }

      inline auto parameter_bind( sqlite3_stmt* statement, int index, const std::string_view& value)
      {
         return code::make( sqlite3_bind_text( statement, index, value.data(), value.size(), SQLITE_STATIC));
      }

      template< casual::concepts::binary::like Range>
      inline auto parameter_bind( sqlite3_stmt* statement, int index, const Range& value)
      {
         return code::make( sqlite3_bind_blob( statement, index, std::data( value), std::size( value), SQLITE_STATIC));
      }

      inline auto parameter_bind( sqlite3_stmt* statement, int index, int value)
      {
         return code::make( sqlite3_bind_int( statement, index, value));
      }

      inline auto parameter_bind( sqlite3_stmt* statement, int index, long value)
      {
         return code::make( sqlite3_bind_int64( statement, index, value));
      }

      inline auto parameter_bind( sqlite3_stmt* statement, int index, long long value)
      {
         return code::make( sqlite3_bind_int64( statement, index, value));
      }

      inline auto parameter_bind( sqlite3_stmt* statement, int index, std::size_t value)
      {
         return code::make( sqlite3_bind_int64( statement, index, value));
      }

      template< typename Rep, typename Period>
      inline auto parameter_bind( sqlite3_stmt* statement, int index, const std::chrono::duration< Rep, Period>& value)
      {
         long long duration = std::chrono::duration_cast< duration_type>( value).count();
         return parameter_bind( statement, index, duration);
      }

      template< typename system_clock, typename duration>
      inline auto parameter_bind( sqlite3_stmt* statement, int index, const std::chrono::time_point< system_clock, duration>& value)
      {
         long long time = std::chrono::time_point_cast< duration_type>( value).time_since_epoch().count();
         return parameter_bind( statement, index, time);
      }


      template< casual::concepts::enumerator T>
      std::error_code parameter_bind( sqlite3_stmt* statement, int column, T value)
      {
         return parameter_bind( statement, column, std::to_underlying( value));
      }

      template< std::integral T>
      bool column_get( sqlite3_stmt* statement, int column, T& value)
      {
         //! handles all integrals over 32b (4B)
         if constexpr( sizeof( T) > 4)
            value = sqlite3_column_int64( statement, column);
         else
            value = sqlite3_column_int( statement, column);

         if( value == T{})
            return sqlite3_column_type( statement, column) != SQLITE_NULL;

         return true;
      }

      inline bool column_get( sqlite3_stmt* statement, int column, std::string& value)
      {
         if( auto text = sqlite3_column_text( statement, column))
         {
            auto size = sqlite3_column_bytes( statement, column);
            value.assign( text, text + size);
            return true;
         }
         return false;
      }

      inline bool column_get( sqlite3_stmt* statement, int column, std::vector< std::byte>& value)
      {
         if( auto blob = sqlite3_column_blob( statement, column))
         {
            auto size = sqlite3_column_bytes( statement, column);

            value.resize( size);
            memcpy( value.data(), blob, size);
            return true;
         }
         return false;
      }


      template< typename T, std::size_t array_size>
      inline bool column_get( sqlite3_stmt* statement, int column, T (&value)[ array_size])
      {
         constexpr auto value_size = sizeof( T) * array_size;

         if( auto blob = sqlite3_column_blob( statement, column))
         {
            std::size_t blob_size = sqlite3_column_bytes( statement, column);
            memcpy( value, blob, value_size > blob_size ? blob_size : value_size);
            return true;
         }
         return false;
      }


      template< typename Clock, typename Duration>
      inline bool column_get( sqlite3_stmt* statement, int column, std::chrono::time_point< Clock, Duration>& value)
      {
         if( sqlite3_column_type( statement, column) != SQLITE_NULL)
         {
            using time_point = std::chrono::time_point< Clock, Duration>;
            duration_type duration{ sqlite3_column_int64( statement, column)};
            value = time_point( duration);
            return true;
         }
         return false;
      }


      template< typename Rep, typename Period>
      inline bool column_get( sqlite3_stmt* statement, int column, std::chrono::duration< Rep, Period>& value)
      {
         if( sqlite3_column_type( statement, column) != SQLITE_NULL)
         {
            duration_type duration{ sqlite3_column_int64( statement, column)};
            value = std::chrono::duration_cast< std::chrono::duration< Rep, Period>>( duration);
            return true;
         }
         return false;
      }

      template< casual::concepts::enumerator T>
      bool column_get( sqlite3_stmt* statement, int column, T& value)
      {
         auto underlying = std::to_underlying( value);
         if( ! column_get( statement, column, underlying))
            return false;
         
         value = static_cast< T>( underlying);
         return true;
      }

      template< typename T>
      inline bool column_get( sqlite3_stmt* statement, int column, std::optional< T>& optional)
      {
         T  value;
         
         if( ! column_get( statement, column, value))
            return false;

         optional = std::move( value);
         return true;
      }

      struct Row
      {
         Row() {};
         Row( const std::shared_ptr< sqlite3_stmt>& statement) : m_statement{ statement} {}

         Row( Row&&) = default;
         Row& operator = ( Row&&) = default;

         template< typename T, int column = 0>
         T get() const
         {
            T value;
            column_get( m_statement.get(), column, value);
            return value;
         }

         template< typename T>
         T get( int column)
         {
            T value;
            column_get( m_statement.get(), column, value);
            return value;
         }

         template< typename T>
         void get( int column, T& value)
         {
            column_get( m_statement.get(), column, value);
         }

         bool null( int column) const
         {
            return sqlite3_column_type( m_statement.get(), column) == SQLITE_NULL;
         }

      private:
         std::shared_ptr< sqlite3_stmt> m_statement;
      };

      namespace row
      {
         struct offset
         {
            constexpr offset( long count) : m_count{ count} {}
            constexpr operator long () const  { return m_count;}
            
            long m_count;
         };

         namespace detail
         {  
            // sentinel
            inline void get( database::Row& row, long index) {} 
            
            template< typename T, typename... Ts> 
            void get( database::Row& row, long index, T&& value, Ts&&... ts)
            {
               row.get( index, std::forward< T>( value));
               get( row, index + 1, std::forward< Ts>( ts)...);
            }
         } // detail 

         template< typename... Ts> 
         void get( database::Row& row, Ts&&... ts)
         {
            detail::get( row, 0, std::forward< Ts>( ts)...);
         }

         template< typename... Ts> 
         void get( database::Row& row, row::offset offset, Ts&&... ts)
         {
            detail::get( row, offset, std::forward< Ts>( ts)...);
         }

      } // row


      struct Statement
      {
         struct Query
         {
            template< typename ...Params>
            Query( const std::shared_ptr< sqlite3>& handle, const std::shared_ptr< sqlite3_stmt>& statement, Params&&... params) : m_handle( handle), m_statement( statement)
            {
               bind( 1, std::forward< Params>( params)...);
            }

            Query( Query&& rhs) = default;

            Query( const Query&) = delete;
            Query& operator = ( const Query&) = delete;

           ~Query()
           {
              sqlite3_reset( m_statement.get());
           }

           std::vector< Row> fetch()
           {
               std::vector< Row> result;
               auto code = code::make( sqlite3_step( m_statement.get()));

               if( code == code::sql::row)
                  result.emplace_back( m_statement);
               else if( code != code::sql::done)
                  code::raise( code, m_handle);

               return result;
           }


            bool fetch( Row& row)
            {
               auto code = code::make( sqlite3_step( m_statement.get()));

               if( code == code::sql::row)
               {
                  row = Row( m_statement);
                  return true;
               }

               if( code == code::sql::done)
                  return false;

               code::raise( code, m_handle);
            }

            void execute()
            {
               auto code = code::make( sqlite3_step( m_statement.get()));

               if( code != code::sql::done)
                  code::raise( code, m_handle);
            }

         private:

            void bind( int index) { /* no op */}

            template< typename T, typename ...Params>
            void bind( int index, T&&value, Params&&... params)
            {
               if( auto code = parameter_bind( m_statement.get(), index, std::forward< T>( value)))
                  code::raise( code, m_handle);
               
               bind( index + 1, std::forward< Params>( params)...);
            }

            std::shared_ptr< sqlite3> m_handle;
            std::shared_ptr< sqlite3_stmt> m_statement;
         };

         Statement() = default;

         Statement( const std::shared_ptr< sqlite3>& handle, const std::string& statement) : m_handle( handle)
         {
            sqlite3_stmt* stmt;

            auto code = code::make( sqlite3_prepare_v2( m_handle.get(), statement.data(), -1, &stmt, nullptr));

            if( code != code::sql::ok)
               code::raise( code, handle);

            m_statement = std::shared_ptr< sqlite3_stmt>( stmt, sqlite3_finalize);
         }

         template< typename ...Params>
         Query query( Params&&... params) const
         {
            return Query{ m_handle, m_statement, std::forward< Params>( params)...};
         }

         template< typename ...Params>
         void execute( Params&&... params) const
         {
            Query{ m_handle, m_statement, std::forward< Params>( params)...}.execute();
         }

         //! @returns the sql that the statement was created by
         inline auto sql() const
         {
            return sqlite3_sql( m_statement.get());
         } 

         inline friend std::ostream& operator << ( std::ostream& out, const Statement& value) 
         {
            if( value.m_statement) 
               return out << "{ statement: " << sqlite3_sql( value.m_statement.get()) << '}';
            else
               return out << "{}";
         }

      private:
         std::shared_ptr< sqlite3> m_handle;
         std::shared_ptr< sqlite3_stmt> m_statement;
      };

      namespace query
      {
         //! @returns a vector with the fetched transformed values
         //! example:  `auto values = sql::database::query::fetch( statement.query( a, b), []( sql::database::Row& row){ /* get values from row and construct and return value */})`
         template< typename F> 
         auto fetch( Statement::Query query, F&& functor)
         {
            sql::database::Row row;
            std::vector< std::decay_t< decltype( functor( row))>> result;

            while( query.fetch( row))
               result.push_back( functor( row));

            return result;
         }

         //! same as query::fetch but only fetches 0..1 rows.
         template< typename F> 
         auto first( Statement::Query query, F&& functor)
         {
            sql::database::Row row;
            std::optional< std::decay_t< decltype( functor( row))>> result;

            if( query.fetch( row))
               result = functor( row);

            return result;
         }

      } // query

      struct Connection
      {
         Connection() = default;

         inline Connection( std::filesystem::path file) 
            : m_handle( open( file)), m_file( std::move( file))
         {
            statement( "PRAGMA foreign_keys=ON;");
            statement( "PRAGMA journal_mode=WAL;");
            statement( "PRAGMA synchronous=NORMAL;");
         }

         inline explicit operator bool() const noexcept { return m_handle && true;}

         inline const std::filesystem::path& file() const noexcept
         {
            return m_file;
         }


         inline Statement precompile( const std::string& statement)
         {
            return Statement{ m_handle, statement};
         }


         template< typename ...Params>
         Statement::Query query( const std::string& statement, Params&&... params)
         {
            return precompile( statement).query( std::forward< Params>( params)...);
         }


         template< typename ...Params>
         void execute( const std::string& statement, Params&&... params)
         {
            query( statement, std::forward< Params>( params)...).execute();
         }

         inline void statement( const char* sql)
         {
            if( auto code = code::make( sqlite3_exec( m_handle.get(), sql, nullptr, nullptr, nullptr)))
               code::raise( code, m_handle);
         }

         //! executes the provided statement and streams the human readable output to `out`
         inline void statement( std::string_view sql, std::ostream& out)
         {
            auto callback = []( void* argument, int count, char** values, char** columns) -> int
            {
               auto out = static_cast< std::ostream*>( argument);

               casual::common::algorithm::for_n( count, [=]( auto index)
               {
                  *out << values[ index] << '|';
               });
               *out << '\n';

               return 0;
            };

            if( auto code = code::make( sqlite3_exec( m_handle.get(), sql.data(), callback, &out, nullptr)))
               code::raise( code, m_handle);
         }

         //! @returns true if the table exists
         inline bool table( const std::string& name)
         {
            return ! query( R"( SELECT * FROM sqlite_master WHERE type = 'table' AND name = ?; )", name).fetch().empty();
         }

         //! @return last rowid
         inline auto rowid() const -> decltype( sqlite3_last_insert_rowid( std::declval<sqlite3*>()))
         {
            return sqlite3_last_insert_rowid( m_handle.get());
         }

         inline auto affected() const -> decltype( sqlite3_changes( std::declval<sqlite3*>()))
         {
            return sqlite3_changes( m_handle.get());
         }

         inline auto pre_statements_path() const { return m_file.parent_path() / casual::common::string::compose( m_file.stem().string(), ".pre.statements");}

         inline void pre_statements( std::ostream& out)
         {
            statements( pre_statements_path(), out);
         }

         inline void post_statements( std::ostream& out)
         {
            statements( m_file.parent_path() / casual::common::string::compose( m_file.stem().string(), ".post.statements"), out);
         }

         inline void begin() const { sqlite3_exec( m_handle.get(), "BEGIN", 0, 0, 0); }
         inline void exclusive_begin() const { sqlite3_exec( m_handle.get(), "BEGIN EXCLUSIVE", 0, 0, 0); }
         inline void rollback() const { sqlite3_exec( m_handle.get(), "ROLLBACK", 0, 0, 0); }
         inline void commit() const { sqlite3_exec( m_handle.get(), "COMMIT", 0, 0, 0); }

         inline friend std::ostream& operator << ( std::ostream& out, const Connection& value) { return out << "{ file: " << value.m_file << '}';}

         inline void pragma_information( std::ostream& out)
         {
            if( ! out)
               return;
            
            auto version =  version::library();

            out << "sqlite version|" << version << "|\n";

            if( version >= 3016000 )
            {
               constexpr std::string_view sql = R"(
SELECT 'pragma_foreign_keys', * FROM "pragma_foreign_keys";
SELECT 'pragma_journal_mode', * FROM "pragma_journal_mode";
SELECT 'pragma_synchronous', * FROM "pragma_synchronous";
)";
               statement( sql, out);
            }
            else
            {
               constexpr std::string_view sql = R"(
PRAGMA foreign_keys;
PRAGMA pragma_journal; 
PRAGMA synchronous;
)";
               statement( sql, out);
            }
         }

      private:

         inline void statements( std::filesystem::path path, std::ostream& out)
         {
            if( ! std::filesystem::exists( path))
               return;

            std::ifstream file( path);
            std::stringstream stream;
            stream << file.rdbuf();

            statement( std::move( stream).str(), out);
         }
         
         inline static std::shared_ptr< sqlite3> open( const std::filesystem::path& file)
         {
            sqlite3* handle = nullptr;
            auto code = code::make( sqlite3_open( file.string().data(), &handle));

            std::shared_ptr< sqlite3> result( handle, sqlite3_close);

            if( code == code::sql::ok)
               return result;
            
            if( code == code::sql::cant_open)
            {
               if( std::filesystem::exists( file))
                  code::raise( code, result);

               if( ! std::filesystem::exists( file.parent_path()))
                  std::filesystem::create_directories( file.parent_path());;

               return open( file);
            }

            code::raise( code, result);
         }

         std::shared_ptr< sqlite3> m_handle;
         std::filesystem::path m_file;
      };

      namespace scoped
      {
         template< typename C>
         struct basic_write
         {
            using connection_type = C;

            basic_write( connection_type& connection) : m_connection( connection)
            {
               m_connection.begin();
            }

            ~basic_write()
            {
               m_connection.commit();

               // TODO: rollback if there is exception in flight?
            }


            connection_type& m_connection;
         };

         using Write = basic_write< Connection>;

         template< typename C>
         basic_write< C> write( C& connection)
         {
            return basic_write< C>( connection);
         }
      } // scoped

      struct Version : casual::common::Compare< Version>
      {
         Version() = default;
         Version( long major, long minor) : major{ major}, minor{ minor} {}
         
         long major = 0;
         long minor = 0;
         
         explicit operator bool() { return *this != Version{};}

         inline auto tie() const -> decltype( std::tie( major, minor)) { return std::tie( major, minor);}

         friend std::ostream& operator << ( std::ostream& out, const Version& value) 
         { 
            return out << "{ major: " << value.major << ", minor: " << value.minor << '}';
         }
      };
      

      namespace version
      {
         inline Version get( Connection& connection)
         {
            Version version;

            if( connection.table( "db_version"))
            {
               auto query = connection.query( "SELECT major, minor FROM db_version ORDER BY major, minor DESC;");

               auto rows = query.fetch();

               
               if( ! rows.empty())
               {
                  auto& row = rows.front();
                  row.get( 0, version.major);
                  row.get( 1, version.minor);
               }
            }

            return version;
         }

         inline void set( Connection& connection, const Version& version)
         {
            if( ! connection.table( "db_version"))
            {
               // we have a new database
               connection.execute(
                  R"( CREATE TABLE db_version 
                  (
                     major        INTEGER  NOT NULL,
                     minor        INTEGER  NOT NULL
                  );
                  )"
               );
            }
            else
            {
               connection.execute( "DELETE FROM db_version;");
            }

            connection.execute( "INSERT INTO db_version VALUES( ?, ?);", version.major, version.minor);
         }

      } // version

   } // database
} // sql



