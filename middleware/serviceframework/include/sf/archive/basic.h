//!
//! casual
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
               inline static bool apply( bool exist, const char* role)
               {
                  if( ! exist)
                  {
                     throw exception::archive::invalid::Node{ "failed to find role in document", CASUAL_NIP( role)};
                  }
                  return exist;
               }
            };

            struct Relaxed
            {
               inline static constexpr bool apply( bool exist, const char* role) { return exist;}
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

            std::tuple< std::size_t, bool> dispatch_container_start( std::size_t size, const char* name) override
            {
               auto result = m_implementation.container_start( size, name);
               policy_type::apply( std::get< 1>( result), name);
               return result;
            }

            void dispatch_container_end( const char* name) override
            {
               m_implementation.container_end( name);
            }

            bool dispatch_serialtype_start( const char* name) override
            {
               return policy_type::apply( m_implementation.serialtype_start( name), name);
            }


            void dispatch_serialtype_end( const char* name) override
            {
               m_implementation.serialtype_end( name);
            }



            template< typename T>
            bool handle_pod( T& value, const char* name)
            {
               return policy_type::apply( m_implementation.read( value, name), name);
            }

            bool pod( bool& value, const char* name) override { return handle_pod( value, name);}
            bool pod( char& value, const char* name) override { return handle_pod( value, name);}
            bool pod( short& value, const char* name) override { return handle_pod( value, name);}
            bool pod( long& value, const char* name) override { return handle_pod( value, name);}
            bool pod( long long& value, const char* name) override { return handle_pod( value, name);}
            bool pod( float& value, const char* name) override { return handle_pod( value, name);}
            bool pod( double& value, const char* name) override { return handle_pod( value, name);}
            bool pod( std::string& value, const char* name) override { return handle_pod( value, name);}
            bool pod( platform::binary_type& value, const char* name) override { return handle_pod( value, name);}

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

            void dispatch_container_start( std::size_t size, const char* name) override
            {
               m_implementation.container_start( size, name);
            }

            void dispatch_container_end( const char* name) override
            {
               m_implementation.container_end( name);
            }

            void dispatch_serialtype_start( const char* name) override
            {
               m_implementation.serialtype_start( name);
            }

            void dispatch_serialtype_end( const char* name) override
            {
               m_implementation.serialtype_end( name);
            }


            template< typename T>
            void handle_pod( const T& value, const char* name)
            {
               m_implementation.write( value, name);
            }

            void pod( const bool value, const char* name) override { handle_pod( value, name);}
            void pod( const char value, const char* name) override { handle_pod( value, name);}
            void pod( const short value, const char* name) override { handle_pod( value, name);}
            void pod( const long value, const char* name) override { handle_pod( value, name);}
            void pod( const long long value, const char* name) override { handle_pod( value, name);}
            void pod( const float value, const char* name) override { handle_pod( value, name);}
            void pod( const double value, const char* name) override { handle_pod( value, name);}
            void pod( const std::string& value, const char* name) override { handle_pod( value, name);}
            void pod( const platform::binary_type& value, const char* name) override { handle_pod( value, name);}

            implementation_type m_implementation;

         };


      }
   }
}



#endif /* CASUAL_BASIC_WRITER_H_ */
