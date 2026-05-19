//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/toml.h"

#include "common/serialize/create.h"
#include "common/binary/span.h"
#include "common/buffer/type.h"

#include "common/serialize/detail/poly.h"

#include <poly/toml.hpp>

namespace casual
{
   namespace common::serialize
   {
      namespace toml
      {
         namespace local
         {
            namespace
            {
               constexpr auto keys() 
               {
                  using namespace std::string_view_literals; 
                  return array::make( "toml"sv, ".toml"sv, buffer::type::toml);
               };

               namespace reader
               {
                  // TOML requires object as root (see reader::adapt)
                  auto adapt( ::poly::node node) -> ::poly::node
                  {
                     if( node.as_object().empty())
                        return ::poly::node{};

                     if( node.as_object().begin()->first.empty())
                        return std::move( node.as_object().begin()->second);

                     return node;
                  }

                  struct Implementation : common::serialize::detail::poly::reader
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                     constexpr static auto keys() { return local::keys();}

                     Implementation( std::istream& value) : base{ adapt( ::poly::toml::parse( value))} {}
                     Implementation( std::string_view value) : base{ adapt( ::poly::toml::parse( value))} {}
                     Implementation( const std::vector<std::byte>& value) : Implementation{ std::string_view{ common::binary::span::to_string_like( value)}} {}
                  };

               } // reader


               namespace writer
               {
                  // TOML requires object as root (see writer::adapt)
                  auto adapt( ::poly::node node) -> ::poly::node
                  {
                     if( node.is_object())
                        return node;
                     
                     return ::poly::node::object{ { {}, std::move( node)}};
                  }

                  struct Implementation : common::serialize::detail::poly::writer
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                     constexpr static auto keys() { return local::keys();}

                     Implementation() = default;

                     void consume( std::ostream& destination)
                     {
                        ::poly::toml::write( adapt( std::move( m_root)), destination);
                     }
                  };

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


         serialize::Writer writer()
         {
            return serialize::create::writer::create< local::writer::Implementation>();
         }

      } // toml

      //
      // explicit instantiations
      template struct create::reader::Registration< toml::local::reader::Implementation>;
      template struct create::writer::Registration< toml::local::writer::Implementation>;

   } // common::serialize

} // casual
