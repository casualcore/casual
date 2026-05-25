//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/yaml.h"

#include "common/serialize/policy.h"
#include "common/serialize/create.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include "common/transcode.h"
#include "common/buffer/type.h"

#include "common/serialize/detail/poly.h"

#include <poly/yaml.hpp>

#include <format>

namespace casual
{
   namespace common::serialize
   {
      namespace yaml
      {
         namespace local
         {
            namespace
            {
               constexpr auto keys() 
               {
                  using namespace std::string_view_literals; 
                  return array::make( "yaml"sv, ".yaml"sv, "yml"sv, ".yml"sv, buffer::type::yaml);
               };

               namespace reader
               {
                  namespace detail
                  {
                     template< typename... types>
                     struct overloaded : types...
                     {
                        using types::operator()...;
                     };

                     template< typename... types>
                     overloaded( types...) -> overloaded< types...>;

                     template< typename type>
                     concept arithmetic = std::integral< type> || std::floating_point< type>;
                  } // detail


                  struct Implementation : common::serialize::detail::poly::reader
                  {
                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                     constexpr static auto keys() { return local::keys();}

                     Implementation( std::string_view value) : base{ ::poly::yaml::parse( value)} {}
                     Implementation( std::istream& value) : base{ ::poly::yaml::parse( value)} {}
                     Implementation( const std::vector<std::byte>& value) : Implementation{ std::string_view{ common::binary::span::to_string_like( value)}} {}

                     using base::read;

                     bool read( std::string& value, const char* const name)
                     {
                        if( auto node = verify< ::poly::node::string, ::poly::node::boolean, ::poly::node::integer, ::poly::node::decimal, ::poly::node::nothing>( name))
                        {
                           std::visit
                           (
                              detail::overloaded
                              {
                                 [&]( ::poly::node::string& data) { value = std::move( data); },
                                 [&]( detail::arithmetic auto& data) { value = std::format( "{}", data); },
                                 [&]( auto& data) {}
                              },
                              *node
                           );

                           return true;
                        }

                        return false;
                     }

                     bool read( platform::binary::type& value, const char* const name)
                     {
                        if( auto node = verify< ::poly::node::binary, ::poly::node::string>( name))
                        {
                           if( node->is_binary())
                              return value = std::move( node->as_binary()), true;
                           
                           return value = transcode::base64::decode( std::move( node->as_string())), true;
                        }
                        return false;
                     }

                     bool read( binary::span::Fixed< std::byte> value, const char* const name)
                     {
                        platform::binary::type data;
                        if( read( data, name))
                        {
                           if( range::size( data) != range::size( value))
                              code::raise::error( code::casual::invalid_node, "binary size mismatch");
                           algorithm::copy( data, std::begin( value));
                           return true;
                        }
                        return false;
                     }
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
                        ::poly::yaml::write( m_root, destination);
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
         }

         namespace consumed
         {    
            serialize::Reader reader( const std::string& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( std::istream& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            serialize::Reader reader( const platform::binary::type& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
         }

         serialize::Writer writer()
         {
            return serialize::create::writer::create< local::writer::Implementation>();
         }

      } // yaml

      namespace create
      {
         namespace reader
         {
            template struct Registration< yaml::local::reader::Implementation>;
         } // reader

         namespace writer
         {
            template struct Registration< yaml::local::writer::Implementation>;
         } // writer
      } // create
      
   } // common::serialize
} // casual

