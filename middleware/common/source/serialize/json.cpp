//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/json.h"

#include "common/serialize/create.h"
#include "common/binary/span.h"
#include "common/buffer/type.h"

#include "common/serialize/detail/poly.h"

#include <poly/json.hpp>

namespace casual
{
   namespace common::serialize
   {
      namespace json
      {
         namespace local
         {
            namespace
            {
               constexpr auto keys() 
               {
                  using namespace std::string_view_literals; 
                  return array::make( "json"sv, ".json"sv, "jsn"sv, ".jsn"sv, buffer::type::json);
               };

               namespace reader
               {
                  struct Implementation : common::serialize::detail::poly::reader
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                     constexpr static auto keys() { return local::keys();}

                     Implementation( std::istream& value) : base{ ::poly::json::parse( value)} {}
                     Implementation( std::string_view value) : base{ ::poly::json::parse( value)} {}
                     Implementation( const std::vector<std::byte>& value) : Implementation{ std::string_view{ common::binary::span::to_string_like( value)}} {}
                  };
               } // reader
               
               
               namespace writer
               {
                  struct Implementation : common::serialize::detail::poly::writer
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                     constexpr static auto keys() { return local::keys();}
                     
                     void consume( std::ostream& destination)
                     {
                        ::poly::json::compact::write( m_root, destination);
                     }
                  };

                  namespace pretty
                  {
                     struct Implementation : writer::Implementation
                     {
                        void consume( std::ostream& destination)
                        {
                           ::poly::json::elegant::write( m_root, destination);
                        }
                     };
                  } // pretty

               } // writer
            } //
         } // local

         namespace strict
         {
            serialize::Reader reader( const std::string& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
            serialize::Reader reader( std::istream& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
            serialize::Reader reader( const platform::binary::type& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
         } // strict

         namespace relaxed
         {    
            serialize::Reader reader( const std::string& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( std::istream& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( const platform::binary::type& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
         } // relaxed

         namespace consumed
         {    
            serialize::Reader reader( const std::string& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( std::istream& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( const platform::binary::type& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
         } // consumed

         namespace pretty
         {
            serialize::Writer writer()
            {
               return serialize::create::writer::create< local::writer::pretty::Implementation>();
            }
         } // pretty

         serialize::Writer writer()
         {
            return serialize::create::writer::create< local::writer::Implementation>();
         }

      } // json
      
      namespace create
      {
         namespace reader
         {
            template struct Registration< json::local::reader::Implementation>;
         } // writer
         namespace writer
         {
            template struct Registration< json::local::writer::pretty::Implementation>;
         } // writer
      } // create

   } // common::serialize
} // casual
