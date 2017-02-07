//!
//! casual
//!

#ifndef SQL_DATABASE_H_
#define SQL_DATABASE_H_


#include <sqlite3.h>


#include "common/algorithm.h"
#include "common/exception.h"
#include "common/file.h"

//
// std
//
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include <chrono>


#include <cstring>

namespace sql
{
   namespace database
   {
      namespace exception
      {
         struct Base : public casual::common::exception::base
         {
            using casual::common::exception::base::base;
         };

         struct Connection : Base
         {
            using Base::Base;
         };

         struct Query : public Base
         {
            using Base::Base;
         };
      }


      struct Blob
      {

         Blob( long size, const char* data) : size( size), data( data) {};

         long size;
         const char* data;
      };

      inline bool parameter_bind( sqlite3_stmt* statement, int index, std::nullptr_t)
      {
         return sqlite3_bind_null( statement, index) == SQLITE_OK;
      }

      inline bool parameter_bind( sqlite3_stmt* statement, int index, const std::string& value)
      {
         return sqlite3_bind_text( statement, index, value.c_str(), value.size(), SQLITE_STATIC) == SQLITE_OK;
      }

      inline bool parameter_bind( sqlite3_stmt* statement, int index, const std::vector< char>& value)
      {
         return sqlite3_bind_blob( statement, index, value.data(), value.size(), SQLITE_STATIC) == SQLITE_OK;
      }

      inline bool parameter_bind( sqlite3_stmt* statement, int index, const Blob& value)
      {
         return sqlite3_bind_blob( statement, index, value.data, value.size, SQLITE_STATIC) == SQLITE_OK;
      }

      template< typename Iter>
      inline bool parameter_bind( sqlite3_stmt* statement, int index, const casual::common::Range< Iter>& value)
      {
         return sqlite3_bind_blob( statement, index, value.data(), value.size(), SQLITE_STATIC) == SQLITE_OK;
      }


      template< typename T, std::size_t array_size>
      inline bool parameter_bind( sqlite3_stmt* statement, int index, T const (&value)[ array_size])
      {
         const auto value_size = sizeof( T) * array_size;

         return sqlite3_bind_blob( statement, index, value, value_size, SQLITE_STATIC) == SQLITE_OK;
      }




      inline bool parameter_bind( sqlite3_stmt* statement, int index, int value)
      {
         return sqlite3_bind_int( statement, index, value) == SQLITE_OK;
      }



      inline bool parameter_bind( sqlite3_stmt* statement, int index, long value)
      {
         return sqlite3_bind_int64( statement, index, value) == SQLITE_OK;
      }

      inline bool parameter_bind( sqlite3_stmt* statement, int index, long long value)
      {
         return sqlite3_bind_int64( statement, index, value) == SQLITE_OK;
      }

      inline bool parameter_bind( sqlite3_stmt* statement, int index, std::size_t value)
      {
         return sqlite3_bind_int64( statement, index, value) == SQLITE_OK;
      }

      template< typename system_clock, typename duration>
      inline bool parameter_bind( sqlite3_stmt* statement, int index, const std::chrono::time_point< system_clock, duration>& value)
      {
         long long time = std::chrono::time_point_cast< std::chrono::microseconds>( value).time_since_epoch().count();
         return parameter_bind( statement, index, time);
      }

      template< typename T>
      inline typename std::enable_if< std::is_enum< T>::value, bool>::type
      parameter_bind( sqlite3_stmt* statement, int column, T value)
      {
         return parameter_bind( statement, column, static_cast< casual::common::traits::underlying_type_t< T>>( value));
      }




      inline void column_get( sqlite3_stmt* statement, int column, long& value)
      {
         value = sqlite3_column_int64( statement, column);
      }

      inline void column_get( sqlite3_stmt* statement, int column, long long& value)
      {
         value = sqlite3_column_int64( statement, column);
      }

      inline void column_get( sqlite3_stmt* statement, int column, std::size_t& value)
      {
         value = sqlite3_column_int64( statement, column);
      }

      inline void column_get( sqlite3_stmt* statement, int column, int& value)
      {
         value = sqlite3_column_int( statement, column);
      }

      inline void column_get( sqlite3_stmt* statement, int column, std::string& value)
      {
         auto text = sqlite3_column_text( statement, column);
         auto size = sqlite3_column_bytes( statement, column);
         value.assign( text, text + size);
      }

      inline void column_get( sqlite3_stmt* statement, int column, std::vector< char>& value)
      {
         auto blob = sqlite3_column_blob( statement, column);
         auto size = sqlite3_column_bytes( statement, column);

         value.resize( size);
         memcpy( value.data(), blob, size);
      }


      template< typename T, std::size_t array_size>
      inline void column_get( sqlite3_stmt* statement, int column, T (&value)[ array_size])
      {
         const auto value_size = sizeof( T) * array_size;

         auto blob = sqlite3_column_blob( statement, column);
         std::size_t blob_size = sqlite3_column_bytes( statement, column);

         memcpy( value, blob, value_size > blob_size ? blob_size : value_size);
      }


      template< typename system_clock, typename duration>
      inline void column_get( sqlite3_stmt* statement, int column, std::chrono::time_point< system_clock, duration>& value)
      {
         if( sqlite3_column_type( statement, column) != SQLITE_NULL)
         {
            using time_point = std::chrono::time_point< system_clock, duration>;
            std::chrono::microseconds us{ sqlite3_column_int64( statement, column)};
            value = time_point( us);
         }
      }

      template< typename T>
      inline typename std::enable_if< std::is_enum< T>::value, void>::type
      column_get( sqlite3_stmt* statement, int column, T& value)
      {
         casual::common::traits::underlying_type_t< T> enum_value;
         column_get( statement, column, enum_value);
         value = static_cast< T>( enum_value);

         // column_get( statement, column, static_cast< enum_type&>( value));
      }

      struct Row
      {
         Row() {};
         Row( const std::shared_ptr< sqlite3_stmt>& statement) : m_statement{ statement} {}

         Row( Row&&) = default;
         Row& operator = ( Row&&) = default;

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
              switch( sqlite3_step( m_statement.get()))
              {
                 case SQLITE_ROW:
                 {
                    result.emplace_back( m_statement);
                    break;
                 }
                 case SQLITE_DONE:
                    break;
                 default:
                    throw exception::Query{ sqlite3_errmsg( m_handle.get()), __FILE__, __LINE__};
              }

              return result;
           }


           bool fetch( Row& row)
           {
              switch( sqlite3_step( m_statement.get()))
              {
                 case SQLITE_ROW:
                 {
                    row = Row( m_statement);
                    return true;
                    break;
                 }
                 case SQLITE_DONE:
                    return false;
                 default:
                    throw exception::Query{ sqlite3_errmsg( m_handle.get()), __FILE__, __LINE__};
              }
           }

            void execute()
            {
               if( sqlite3_step( m_statement.get()) != SQLITE_DONE)
               {
                  throw exception::Query{ sqlite3_errmsg( m_handle.get()), __FILE__, __LINE__};
               }
            }

         private:

           void bind( int index) { /* no op */}

           template< typename T, typename ...Params>
           void bind( int index, T&&value, Params&&... params)
           {
              if( ! parameter_bind( m_statement.get(), index, std::forward< T>( value)))
              {
                 throw exception::Query{ sqlite3_errmsg( m_handle.get()) + std::string{ " index: "} + std::to_string( index), __FILE__, __LINE__};
              }
              bind( index + 1, std::forward< Params>( params)...);
           }

            std::shared_ptr< sqlite3> m_handle;
            std::shared_ptr< sqlite3_stmt> m_statement;
         };

         Statement() = default;

         Statement( const std::shared_ptr< sqlite3>& handle, const std::string& statement) : m_handle( handle)
         {
            sqlite3_stmt* stmt;

            if( sqlite3_prepare_v2( m_handle.get(), statement.data(), -1, &stmt, nullptr) != SQLITE_OK)
            {
               throw exception::Query{ sqlite3_errmsg( handle.get()), __FILE__, __LINE__};
            }
            m_statement = std::shared_ptr< sqlite3_stmt>( stmt, sqlite3_finalize);
         }

         template< typename ...Params>
         Query query( Params&&... params)
         {
            return Query{ m_handle, m_statement, std::forward< Params>( params)...};
         }

         template< typename ...Params>
         void execute( Params&&... params)
         {
            Query{ m_handle, m_statement, std::forward< Params>( params)...}.execute();
         }

      private:
         std::shared_ptr< sqlite3> m_handle;
         std::shared_ptr< sqlite3_stmt> m_statement;
      };

      struct Connection
      {
         Connection( std::string filename) : m_file( std::move( filename)), m_handle( open( m_file))
         {
            //sqlite3_exec( m_handle.get(), "PRAGMA journal_mode = WAL;", 0, 0, 0);
         }

         std::shared_ptr< sqlite3> open( const std::string& file)
         {

            sqlite3* p_handle = nullptr;

            auto result = sqlite3_open( file.c_str(), &p_handle);

            std::shared_ptr< sqlite3> handle( p_handle, sqlite3_close);

            switch( result)
            {
               case SQLITE_OK:
                  break;
               case SQLITE_CANTOPEN:
               {
                  if( casual::common::file::exists( file))
                  {
                     throw exception::Connection( sqlite3_errmsg( handle.get()), CASUAL_NIP( file), CASUAL_NIP( result));
                  }
                  else
                  {
                     auto created = casual::common::directory::create( casual::common::directory::name::base( file));

                     if (!created)
                     {
                        throw exception::Connection( sqlite3_errmsg( handle.get()), CASUAL_NIP( file), CASUAL_NIP( result));
                     }

                     return open( file);
                  }
                  break;
               }
               default:
               {
                  throw exception::Connection( sqlite3_errmsg( handle.get()), CASUAL_NIP( file), CASUAL_NIP( result));
               }


            }

            return handle;
         }

         std::string file() const
         {
            return m_file;
            /*
            auto path = sqlite3_db_filename( m_handle.get(), "main");
            if( ! path)
            {
               return {};
            }
            return path;
            */
         }


         Statement precompile( const std::string& statement)
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


         //!
         //! @return last rowid
         //!
         auto rowid() const -> decltype( sqlite3_last_insert_rowid( std::declval<sqlite3*>()))
         {
            return sqlite3_last_insert_rowid( m_handle.get());
         }

         auto affected() const -> decltype( sqlite3_changes( std::declval<sqlite3*>()))
         {
            return sqlite3_changes( m_handle.get());
         }

         void begin() const { sqlite3_exec( m_handle.get(), "BEGIN", 0, 0, 0); }
         void exclusive_begin() const { sqlite3_exec( m_handle.get(), "BEGIN EXCLUSIVE", 0, 0, 0); }
         void rollback() const { sqlite3_exec( m_handle.get(), "ROLLBACK", 0, 0, 0); }
         void commit() const { sqlite3_exec( m_handle.get(), "COMMIT", 0, 0, 0); }

      private:
         std::string m_file;
         std::shared_ptr< sqlite3> m_handle;
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

   } // database
} // sql




#endif /* DATABASE_H_ */
