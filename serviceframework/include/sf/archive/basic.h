//!
//! casual_basic_writer.h
//!
//! Created on: Oct 14, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BASIC_WRITER_H_
#define CASUAL_BASIC_WRITER_H_

#include "sf/archive/archive.h"
#include "sf/exception.h"

#include <utility>

namespace casual
{
   namespace sf
   {
      namespace archive
      {

         namespace policy
         {
            struct Strict
            {
               inline static constexpr bool check() { return true;}
               inline static void apply( bool exist, const char* role)
               {
                  if( ! exist)
                  {
                     throw exception::archive::invalid::Node{ "archive - failed to find role '" + std::string{ role} + "' in document"};
                  }
               }
            };

            struct Relaxed
            {
               inline static constexpr bool check() { return false;}
               inline static void apply( bool exist, const char* role) {}
            };

         } // policy


         template< typename I, typename P>
         class basic_reader : public Reader
         {

         public:

            using implementation_type = I;
            using policy_type = P;


            template< typename... Arguments>
            basic_reader( Arguments&&... arguments)
             : m_implementation( std::forward< Arguments>( arguments)...)
               {

               }

            basic_reader( basic_reader&&) = default;

            const implementation_type& implemenation() const
            {
               return m_implementation;
            }


         private:


            std::size_t container_start( std::size_t size, const char* name)
            {
               auto result = m_implementation.container_start( size, name);
               policy_type::apply( std::get< 1>( result), name);
               return std::get< 0>( result);
            }

            void container_end( const char* name)
            {
               m_implementation.container_end( name);
            }

            void serialtype_start( const char* name)
            {
               policy_type::apply( m_implementation.serialtype_start( name), name);
            }


            void serialtype_end( const char* name)
            {
               m_implementation.serialtype_end( name);
            }



            template< typename T>
            void handle_pod( T& value, const char* name)
            {
               policy_type::apply( m_implementation.read( value, name), name);
            }

            void pod( bool& value, const char* name) { handle_pod( value, name);}
            void pod( char& value, const char* name) { handle_pod( value, name);}
            void pod( short& value, const char* name) { handle_pod( value, name);}
            void pod( long& value, const char* name) { handle_pod( value, name);}
            void pod( long long& value, const char* name) { handle_pod( value, name);}
            void pod( float& value, const char* name) { handle_pod( value, name);}
            void pod( double& value, const char* name) { handle_pod( value, name);}
            void pod( std::string& value, const char* name) { handle_pod( value, name);}
            void pod( platform::binary_type& value, const char* name) { handle_pod( value, name);}

            implementation_type m_implementation;
         };


         template< typename I>
         class basic_writer : public Writer
         {

         public:

            using implementation_type = I;

            template< typename... Arguments>
            basic_writer( Arguments&&... arguments)
             : m_implementation( std::forward< Arguments>( arguments)...)
               {

               }

            basic_writer( basic_writer&&) = default;

            const implementation_type& implementation() const
            {
               return m_implementation;
            }

         private:

            std::size_t container_start( std::size_t size, const char* name)
            {
               return m_implementation.container_start( size, name);
            }

            void container_end( const char* name)
            {
               m_implementation.container_end( name);
            }

            void serialtype_start( const char* name)
            {
               m_implementation.serialtype_start( name);
            }

            void serialtype_end( const char* name)
            {
               m_implementation.serialtype_end( name);
            }


            template< typename T>
            void handle_pod( const T& value, const char* name)
            {
               m_implementation.write( value, name);
            }

            void pod( const bool value, const char* name) { handle_pod( value, name);}
            void pod( const char value, const char* name) { handle_pod( value, name);}
            void pod( const short value, const char* name) { handle_pod( value, name);}
            void pod( const long value, const char* name) { handle_pod( value, name);}
            void pod( const long long value, const char* name) { handle_pod( value, name);}
            void pod( const float value, const char* name) { handle_pod( value, name);}
            void pod( const double value, const char* name) { handle_pod( value, name);}
            void pod( const std::string& value, const char* name) { handle_pod( value, name);}
            void pod( const platform::binary_type& value, const char* name) { handle_pod( value, name);}

            implementation_type m_implementation;

         };


      }
   }
}



#endif /* CASUAL_BASIC_WRITER_H_ */
