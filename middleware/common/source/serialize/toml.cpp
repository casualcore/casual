//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/toml.h"

#include "common/serialize/create.h"

#include "common/transcode.h"
#include "common/buffer/type.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <toml++/toml.hpp>

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
                  class Implementation
                  {
                  public:

                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                     constexpr static auto keys() { return local::keys();}

                     Implementation( std::istream& toml) : m_table{ ::toml::parse( toml)} {}

                     Implementation( std::string_view toml) : m_table{ ::toml::parse( toml)} {}

                     Implementation( const std::vector<std::byte>& toml) : m_table{ ::toml::parse( std::string_view{ common::binary::span::to_string_like( toml)})} {}

                     std::tuple< platform::size::type, bool> container_start( const platform::size::type size, const char* const name)
                     {
                        if( append( name, &::toml::node::is_array))
                        {
                           const auto array = m_stack.back()->as_array();

                           for( const auto& node : range::reverse( range::make( array->begin(), array->end())))
                              m_stack.push_back( &node);
                           
                           return { array->size(), true};
                        }

                        return {};
                     }

                     void container_end( const char* const name) 
                     {
                        m_stack.pop_back();
                     }

                     bool composite_start( const char* const name)
                     {
                        //
                        // To support (possible unnamed) other types than table as root
                        if( m_stack.empty())
                           return m_stack.push_back( &m_table), true;

                        return append(name, &::toml::node::is_table);
                     }

                     void composite_end(  const char* const name)
                     {
                        m_stack.pop_back();
                     }

                     bool read( bool& value, const char* name)
                     {
                        return assign( name, static_cast< const ::toml::value<bool>*(::toml::node::*)() const>( &::toml::node::as_boolean), value);
                     }

                     template<typename T>
                     bool read( T& value, const char* name) requires std::is_integral_v< T>
                     {
                        return assign( name, static_cast< const ::toml::value<std::int64_t>*(::toml::node::*)() const>( &::toml::node::as_integer), value);
                     }
                     
                     template<typename T>
                     bool read( T& value, const char* name) requires std::is_floating_point_v< T>
                     {
                        return assign( name, static_cast< const ::toml::value<double>*(::toml::node::*)() const>( &::toml::node::as_floating_point), value);
                     }
                     
                     bool read( std::string_view& value, const char* name)
                     {
                        return assign( name, static_cast< const ::toml::value<std::string>*(::toml::node::*)() const>( &::toml::node::as_string), value);
                     }

                     bool read( char& value, const char* name)
                     {
                        std::string_view data;
                        if( read( data, name))
                           return value = transcode::utf8::string::decode( data).at( 0), true;

                        return false;
                     }

                     bool read( std::string& value, const char* name)
                     {
                        std::string_view data;
                        if( read( data, name))
                           return value = transcode::utf8::string::decode( data), true;

                        return false;
                     }

                     bool read( std::u8string& value, const char* name)
                     {
                        std::string_view data;
                        if( read( data, name))
                           return value = transcode::utf8::cast( data), true;

                        return false;
                     }

                     bool read( platform::binary::type& value, const char* name)
                     {
                        std::string_view data;
                        if( read( data, name))
                           return value = transcode::base64::decode( data), true;

                        return false;
                     }

                     bool read( binary::span::Fixed< std::byte> value, const char* name)
                     {
                        std::string_view data;
                        if( read( data, name))
                        {
                           const auto binary = transcode::base64::decode( data);
                           if( range::size( binary) != range::size( value))
                              code::raise::error( code::casual::invalid_node, "binary size mismatch");

                           algorithm::copy( binary, std::begin( value));

                           return true;
                        }

                        return false;
                     }

                     policy::canonical::Representation canonical()
                     {
                        return {};
                     }

                  private:

                     auto search( const char* name) -> const ::toml::node*
                     {
                        //
                        // To support (possible unnamed) other types than table as root
                        if( m_stack.empty())
                        {
                           m_stack.push_back( &m_table);
                           name = name ? name : "";
                        }
                        
                        if( name)
                        {
                           auto node = m_stack.back()->as_table()->find( name);

                           if( node != m_stack.back()->as_table()->end())
                              return &node->second;
                           else
                              return nullptr;
                        }

                        auto result = m_stack.back();
                        m_stack.pop_back();
                        return result;
                     }


                     bool assign( const char* name, const auto& type, auto& data)
                     {
                        if( const auto node = search( name))
                        {
                           if( const auto result = std::invoke< decltype( type)>( type, node))
                           {
                              return data = static_cast< const std::decay_t< decltype( data)>&>( result->get()), true;
                           }
                        }

                        return false;
                     }

                     bool append( const char* name, const auto& type)
                     {
                        if( auto node = search( name))
                           if( std::invoke< decltype( type)>( type, node))
                              return m_stack.push_back( node), true;

                        return false;
                     }

                  private:

                     ::toml::table m_table;
                     std::vector< const ::toml::node*> m_stack;
                  };

               } // reader


               namespace writer
               {
                  
                  class Implementation
                  {
                  public:

                     constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

                     constexpr static auto keys() { return local::keys();}

                     Implementation() = default;

                     platform::size::type container_start( const platform::size::type size, const char* const name)
                     {
                        handle_object(name, ::toml::array{})->as_array()->reserve( size);

                        return size;
                     }
                     
                     void container_end( const char* const name)
                     {
                        m_stack.pop_back();
                     }

                     void composite_start( const char* const name)
                     {
                        //
                        // To support (possible unnamed) other types than table as root
                        if( m_stack.empty())
                           return m_stack.push_back( &m_table);

                        handle_object( name, ::toml::table{});
                     }

                     void composite_end( const char* const name)
                     {
                        m_stack.pop_back();
                     }

                     void write( auto&& data, const char* const name)
                     {
                        handle_member( name, std::forward< decltype( data)>( data));
                     }

                     void write( char data, const char* const name)
                     {
                        handle_member( name, transcode::utf8::string::encode( std::string{ data}));
                     }

                     void write( const std::string& data, const char* const name)
                     {
                        handle_member( name, transcode::utf8::string::encode( data));
                     }

                     void write( const std::u8string& data, const char* const name)
                     {
                        handle_member( name, transcode::utf8::cast( data));
                     }

                     void write( std::span< const std::byte> data, const char* const name)
                     {
                        handle_member( name, transcode::base64::encode( data));
                     }

                     void write( binary::span::Fixed< const std::byte> data, const char* const name)
                     {
                        handle_member( name, transcode::base64::encode( data));
                     }

                     void consume( std::ostream& destination)
                     {
                        destination << m_table;
                     }

                  private:

                     auto handle_member( const char* name, auto&& data) -> ::toml::node*
                     {
                        //
                        // To support (possible unnamed) other types than table as root
                        if( m_stack.empty())
                        {
                           m_stack.push_back( &m_table);
                           name = name ? name : "";
                        }

                        if( m_stack.back()->is_table())
                           return &m_stack.back()->as_table()->insert( name, std::move( data)).first->second;

                        m_stack.back()->as_array()->push_back( data);
                        return &m_stack.back()->as_array()->back();
                     }

                     auto handle_object( const char* name, auto&& data) -> ::toml::node*
                     {
                        m_stack.push_back( handle_member( name, std::move( data)));

                        if( m_stack.back()) return m_stack.back();

                        return m_stack.pop_back(), nullptr;
                     }

                  private:

                     ::toml::table m_table;
                     std::vector< ::toml::node*> m_stack;

                  }; // Implementation

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
