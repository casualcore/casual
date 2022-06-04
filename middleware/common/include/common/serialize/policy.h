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
#include "common/string/compose.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <tuple>

namespace casual
{
   namespace common::serialize::policy
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
            inline auto operator() () &
            {
               return common::algorithm::unique( common::algorithm::sort( m_canonical));
            }

            template< typename T>
            inline void attribute( T&& name)
            {
               // Note:  this is the only place where we actually create a canonical name. 

               // if the stack is empty, we 'fake' a composite, to have a 'logical' placeholder for the attribute
               if( m_stack.empty())
                     m_stack.emplace_back( "{}");

               auto view = Representation::view( name);

               if( ! view.empty())
                  m_canonical.push_back( string::compose( get_path(), '.', view));
               else
               {
                  // if we've got the canonical name already, we don't add it again
                  auto path = get_path();

                  if( m_canonical.empty() || range::back( m_canonical) != path)
                     m_canonical.push_back( std::move( path));
               }
            }

            template< typename T>
            inline void container_start( T&& name)
            {
               container_composite_start( std::forward< T>( name), "[]");
            }

            inline void container_end() { container_composite_end();}

            template< typename T>
            inline void composite_start( T&& name)
            {
               container_composite_start( std::forward< T>( name), "{}");
            }

            inline void composite_end() { container_composite_end();}

         private:

            static std::string_view view( const char* name)
            {
               if( name)
                  return name;
               return {};
            }

            static std::string_view view( const std::string& name)
            {
               return name;
            }

            template< typename T>
            inline void container_composite_start( T&& name, std::string_view postfix)
            {
               auto view = Representation::view( name);
               if( ! view.empty())
               {
                  // if the stack is empty, we 'fake' a composite, to have a 'logical' placeholder for the name
                  if( m_stack.empty())
                     m_stack.emplace_back( "{}");

                  m_stack.push_back( string::compose( name, postfix));
               }
               else  
                  m_stack.emplace_back( postfix);
            }

            inline void container_composite_end()
            {
               assert( ! m_stack.empty());
               m_stack.pop_back();
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
               m_canonical.container_start( name);

            return result;
         }

         void container_end( const char* name)
         {
            Implementation::container_end( name);
            m_canonical.container_end();
         }

         inline bool composite_start( const char* name)
         {
            auto result = Implementation::composite_start( name);

            if( result)
               m_canonical.composite_start( name);
            
            return result;
         }

         void composite_end( const char* name) 
         {
            Implementation::composite_end( name);
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
            Trace trace{ "common::serialize::policy::validate"};
            auto canonical = Implementation::canonical();
            auto source = canonical();
            auto consumed = m_canonical();

            if( auto not_consumed = std::get< 1>( common::algorithm::intersection( source, consumed)))
            {
               common::log::line( verbose::log, "source: ", source);
               common::log::line( verbose::log, "consumed: ", consumed);
               code::raise::error( code::casual::invalid_configuration, "not all information consumed from source - ", not_consumed);
            }
         }
      private:
         policy::canonical::Representation m_canonical;
      };

   } // common::serialize::policy
} // casual
