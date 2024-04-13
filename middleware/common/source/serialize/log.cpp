//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/log.h"
#include "common/serialize/create.h"

#include "common/transcode.h"


#include <iostream>
#include <iomanip>
#include <locale>
#include <algorithm>

namespace casual
{
   namespace common::serialize::log
   {
      namespace local
      {
         namespace
         {         
            struct Implementation
            {
               inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

               static constexpr auto keys() 
               {
                  using namespace std::string_view_literals; 
                  return array::make( ""sv, "log"sv);
               };

               platform::size::type container_start( const platform::size::type size, const char* name)
               {
                  prefix( name);
                  
                  if( size == 0)
                     m_output << "[]";
                        
                  m_output << '\n';
                  
                  start( "  - ");
                  return size;
               }

               void container_end( const char*)
               {
                  end();
               }

               void composite_start( const char* name)
               {
                  prefix( name) << "{\n";
                  start( "  ");
               }
               
               void composite_end(  const char*)
               {
                  end( "}\n");
               }

               template<typename T>
               void write( T value, const char* name)
               {
                  prefix( name) << value << '\n';
               }

               void write( bool value, const char* name)
               {
                  prefix( name) << ( value ? "true" : "false") << '\n';
               }

               void write( char value, const char* name)
               {
                  prefix( name) << "'" << value << "'" << '\n';
               }

               void write( const std::string& value, const char* name)
               {
                  prefix( name) << std::quoted( value) << '\n';
               }

               void write( const std::u8string& value, const char* name)
               {
                  write( transcode::utf8::decode( value), name);
               }

               void write( view::immutable::Binary value, const char* name)
               {
                  if( value.size() > 32) 
                     transcode::hex::encode( prefix( name), view::binary::make( std::begin( value), 32)) << "... (size: " << value.size() << ")\n";
                  else
                     transcode::hex::encode( prefix( name),value) << '\n';
               }

               void write( const platform::binary::type& value, const char* name)
               {
                  write( view::binary::make( value), name);
               }

               void consume( std::string& destination)
               {
                  std::ostringstream output;
                  std::swap( output, m_output);
                  destination = std::move( output).str();
               }

            private:

               void start( const char* prefix)
               {
                  m_prefix.emplace_back( prefix);
               }

               void end()
               {
                  m_prefix.pop_back();
               }

               template< typename... Ts>
               void end( Ts&&... ts)
               {
                  m_prefix.pop_back();

                  whitespace( m_prefix);

                  common::stream::write( m_output, std::forward< Ts>( ts)...);
               }

               std::ostream& prefix( const char* name)
               {
                  if( ! m_prefix.empty())
                  {
                     whitespace( range::make( std::begin( m_prefix), m_prefix.size() - 1));
                     m_output << m_prefix.back();
                  }

                  if( name)
                     m_output << name << ": ";
                  
                  return m_output;
               }

               template< typename R> 
               void whitespace( R&& range)
               {
                  auto count = algorithm::accumulate( range, 0, []( auto value, auto& range){ return value + range.size();});
                  m_output << std::setw( count) << "";
               }

               std::ostringstream m_output;
               std::vector< std::string_view> m_prefix;
            };
         } // <unnamed>
      } // local


      Writer writer()
      {
         return Writer::emplace< local::Implementation>();
      }
      
   } // common::serialize::log


   namespace common::serialize::create::writer
   {
      template struct Registration< log::local::Implementation>;
   } // common::serialize::create::writer
} // casual
