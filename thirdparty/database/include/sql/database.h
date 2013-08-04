//!
//! database.h
//!
//! Created on: Jul 24, 2013
//!     Author: Lazan
//!

#ifndef DATABASE_H_
#define DATABASE_H_


#include <sqlite3.h>

#include <stdexcept>

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

      inline bool parameter_bind( sqlite3_stmt* statement, int index, const std::string& value)
      {
         return sqlite3_bind_text( statement, index, value.data(), value.size(), SQLITE_TRANSIENT) == SQLITE_OK;
      }

      inline bool parameter_bind( sqlite3_stmt* statement, int index, const std::vector< char>& value)
      {
         return sqlite3_bind_blob( statement, index, value.data(), value.size(), SQLITE_TRANSIENT) == SQLITE_OK;
      }

      /*
      bool parameter_bind( sqlite3_stmt* statement, int index, int value)
      {
         return sqlite3_bind_int( statement, index, value) == SQLITE_OK;
      }
      */


      inline bool parameter_bind( sqlite3_stmt* statement, int index, long value)
      {
         return sqlite3_bind_int64( statement, index, value) == SQLITE_OK;
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


      struct Row
      {
         Row( sqlite3_stmt* statement) : m_statement{ statement} {}

         template< typename T>
         T get( int column)
         {
            T value;
            column_get( m_statement, column, value);
            return value;
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

            bind( 0, std::forward< Params>( params)...);

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

         std::vector< Row> fetch()
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

      private:

         void bind( int index) { /* no op */}

         template< typename T, typename ...Params>
         void bind( int index, T&&value, Params&&... params)
         {
            if( ! parameter_bind( m_statement, index, std::forward< T>( value)))
            {
               throw exception::Query{ sqlite3_errmsg( m_handle)};
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
         Query query( const std::string& statement, Params&&... params)
         {
            return Query{ m_handle, statement, std::forward< Params>( params)...};
         }


         template< typename ...Params>
         void execute( const std::string& statement, Params&&... params)
         {
            Query{ m_handle, statement, std::forward< Params>( params)...}.execute();
         }

      private:

         sqlite3* m_handle = nullptr;
      };

   } // database
} // sql




#endif /* DATABASE_H_ */
