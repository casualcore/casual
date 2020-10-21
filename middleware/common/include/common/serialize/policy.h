//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/archive.h"
#include "common/serialize/log.h"
#include "common/log/category.h"
#include "common/log.h"
#include "common/string.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <tuple>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace policy
         {
            template< typename Implementation>
            struct Strict : Implementation
            {
               using Implementation::Implementation;

               inline static bool apply( bool exist, const char* role)
               {
                  if( ! exist)
                     code::raise::error( code::casual::invalid_node, "failed to find role in document - role: ", role);

                  return exist;
               }

               inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name)
               {
                  auto result = Implementation::container_start( size, name);
                  apply( std::get< 1>( result), name);
                  return result;
               }

               inline bool composite_start( const char* name)
               {
                  return apply( Implementation::composite_start( name), name);
               }

               template< typename T>
               bool read( T& value, const char* name)
               {
                  return apply( Implementation::read( value, name), name);
               }
            };

            template< typename Implementation>
            struct Relaxed : Implementation
            {
               using Implementation::Implementation;
            };


            namespace canonical
            {
               struct Representation
               {
                  inline auto canonical() &
                  {
                     return common::algorithm::unique( common::algorithm::sort( m_canonical));
                  }

                  inline void attribute( const char* name)
                  {
                     auto path = get_path();
   
                     if( ! path.empty())
                        m_canonical.push_back( std::move( path) + '.' + get_name( name));
                     else 
                        m_canonical.emplace_back( get_name( name));
                  }


                  inline void attribute( const std::string& name)
                  {
                     attribute( name.data());
                  }

                  inline void composite_start( const char* name)
                  {
                     m_stack.emplace_back( get_name( name));
                  }

                  inline void composite_start( const std::string& name)
                  {
                     composite_start( name.data());
                  }

                  inline void composite_end()
                  {
                     if( ! m_stack.empty())
                        m_stack.pop_back();
                  }

                  inline friend std::ostream& operator << ( std::ostream& out, const Representation& value)
                  {
                     return stream::write( out, "{ stack: ", value.m_stack, ", canonical: ", value.m_canonical, '}');
                  }

               private:

                  const char* get_name( const char* name) const
                  {
                     // we assume that all 'null' names are unnamed stuff
                     // in a container.
                     return ( name ? name : "element");
                  }
                  
                  std::string get_path() const
                  {
                     return string::join( m_stack, ".");
                  }

                  std::vector< std::string> m_stack;
                  std::vector< std::string> m_canonical;
               };

            } // canonical


            template< typename Implementation>
            struct Consumed : Implementation
            {
               using Implementation::Implementation;


               inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name)
               {
                  auto result = Implementation::container_start( size, name);
                  
                  if( std::get< 1>( result))
                     m_canonical.composite_start( name);

                  return result;
               }

               void container_end( const char* name)
               {
                  Implementation::container_end( name);
                  m_canonical.composite_end();
               }

               inline bool composite_start( const char* name)
               {
                  auto result = Implementation::composite_start( name);

                  if( result)
                     m_canonical.composite_start( name);
                  
                  return result;
               }

               void composite_end(  const char* name) 
               {
                  Implementation::composite_end(  name);
                  m_canonical.composite_end();
               }

               template< typename T>
               bool read( T& value, const char* name)
               {
                  auto result = Implementation::read( value, name);
                  
                  if( result)
                     m_canonical.attribute( name);

                  return result;
               }

               void validate()
               {
                  auto source = Implementation::canonical();
                  auto available = source.canonical();
                  auto consumed = m_canonical.canonical();

                  common::log::line( verbose::log, "available: ", available);
                  common::log::line( verbose::log, "consumed: ", consumed);

                  if( auto not_consumed = common::algorithm::difference( available, consumed))
                     code::raise::error( code::casual::invalid_configuration, "not all information consumed from source - ", not_consumed);
               }
            private:
               policy::canonical::Representation m_canonical;
            };


         } // policy
      } // serialize
   } // common
} // casual
