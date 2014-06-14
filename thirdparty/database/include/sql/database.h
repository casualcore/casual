//!
//! database.h
//!
//! Created on: Jul 24, 2013
//!     Author: Lazan
//!

#ifndef DATABASE_H_
#define DATABASE_H_


#include <sqlite3.h>


#include "common/algorithm.h"

//
// std
//
#include <stdexcept>
#include <vector>
#include <string>



#include <cstring>

namespace sql
{
   namespace database
   {
      namespace exception
      {
         struct Connection : public std::runtime_error
         {
            Connection( const std::string& information) : std::runtime_error( information) {}
         };

         struct Query : public std::runtime_error
         {
            Query( const std::string& information) : std::runtime_error( information) {}
         };
      }


      struct Blob
      {

         Blob( long size, const char* data) : size( size), data( data) {};

         long size;
         const char* data;
      };


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
         auto blob_size = sqlite3_column_bytes( statement, column);

         memcpy( value, blob, value_size > blob_size ? blob_size : value_size);
      }


      struct Row
      {
         Row() {};
         Row( sqlite3_stmt* statement) : m_statement{ statement} {}

         Row( Row&&) = default;
         Row& operator = ( Row&&) = default;

         template< typename T>
         T get( int column)
         {
            T value;
            column_get( m_statement, column, value);
            return value;
         }

         template< typename T>
         void get( int column, T& value)
         {
            column_get( m_statement, column, value);
         }



      private:
         sqlite3_stmt* m_statement = nullptr;
      };

      struct Query
      {
         template< typename ...Params>
         Query( sqlite3* handle, const std::string& statement, Params&&... params) : m_handle( handle)
         {
            if( sqlite3_prepare_v2( m_handle, statement.data(), -1, &m_statement, nullptr) != SQLITE_OK)
            {
               throw exception::Query{ sqlite3_errmsg( handle)};
            }

            bind( 1, std::forward< Params>( params)...);

         }

         Query( Query&& rhs)
         {
            std::swap( m_statement, rhs.m_statement);
            std::swap( m_handle, rhs.m_handle);
         }

         Query( const Query&) = delete;
         Query& operator = ( const Query&) = delete;

        ~Query()
        {
           sqlite3_finalize( m_statement);
        }

        void execute()
        {
           if( sqlite3_step( m_statement) != SQLITE_DONE)
           {
              throw exception::Query{ sqlite3_errmsg( m_handle)};
           }
        }

         std::vector< Row> fetch() &
         {
            std::vector< Row> result;
            switch( sqlite3_step( m_statement))
            {
               case SQLITE_ROW:
               {
                  result.emplace_back( m_statement);
                  break;
               }
               case SQLITE_DONE:
                  break;
               default:
                  throw exception::Query{ sqlite3_errmsg( m_handle)};
            }

            return result;
         }


         bool fetch( Row& row) &
         {
            switch( sqlite3_step( m_statement))
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
                  throw exception::Query{ sqlite3_errmsg( m_handle)};
            }
         }

      private:

         void bind( int index) { /* no op */}

         template< typename T, typename ...Params>
         void bind( int index, T&&value, Params&&... params)
         {
            if( ! parameter_bind( m_statement, index, std::forward< T>( value)))
            {
               throw exception::Query{ sqlite3_errmsg( m_handle) + std::string{ " index: "} + std::to_string( index)};
            }
            bind( index + 1, std::forward< Params>( params)...);
         }


        sqlite3_stmt* m_statement = nullptr;
        sqlite3* m_handle = nullptr;
      };

      struct Connection
      {
         Connection( const std::string &filename)
         {
            if( sqlite3_open( filename.data(), &m_handle) != SQLITE_OK)
               //&& sql("PRAGMA foreign_keys = ON;"))
            {
               throw exception::Connection( sqlite3_errmsg( m_handle));
            }
         }

         ~Connection()
         {
            sqlite3_close( m_handle);
         }

         Connection( Connection&& rhs)
         {
            std::swap( m_handle, rhs.m_handle);
         }

         Connection( const Connection&) = delete;
         Connection& operator = ( const Connection&) = delete;


         template< typename ...Params>
         Query query( const std::string& statement, Params&&... params) &
         {
            return Query{ m_handle, statement, std::forward< Params>( params)...};
         }


         template< typename ...Params>
         void execute( const std::string& statement, Params&&... params) &
         {
            Query{ m_handle, statement, std::forward< Params>( params)...}.execute();
         }


         //!
         //! @return last rowid
         //!
         auto rowid() const -> decltype( sqlite3_last_insert_rowid( std::declval<sqlite3*>()))
         {
            return sqlite3_last_insert_rowid( m_handle);
         }

         void begin() const { sqlite3_exec( m_handle, "BEGIN", 0, 0, 0); }
         void rollback() const { sqlite3_exec( m_handle, "ROLLBACK", 0, 0, 0); }
         void commit() const { sqlite3_exec( m_handle, "COMMIT", 0, 0, 0); }

      private:

         sqlite3* m_handle = nullptr;
      };

   } // database
} // sql




#endif /* DATABASE_H_ */
