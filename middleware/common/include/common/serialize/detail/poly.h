//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"
#include "common/transcode.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/serialize/archive.h"

#include <ranges>
#include <vector>
#include <cassert>

#include <poly/node.hpp>

namespace casual::common::serialize::detail::poly
{
   class reader
   {
   public:

      struct official
      {
         auto operator() ( const ::poly::node& root)
         {
            std::visit( *this, root);
            return std::exchange( m_info, {});
         }

         void operator() ( const auto&) 
         {
            m_info.attribute( m_name);
         }

         void operator() ( const ::poly::node::nothing&) 
         {

         }

         void operator() ( const ::poly::node::array& node)
         {
            m_info.container_start( m_name);
            for( const auto& item : node)
            {
               m_name = nullptr;
               std::visit( *this, item);
            }
            m_info.container_end();
         }

         void operator() ( const ::poly::node::object& node)
         {
            m_info.composite_start( m_name);
            for( const auto& [ name, data] : node)
            {
               m_name = name.data();
               std::visit( *this, data);
            }
            m_info.composite_end();
         }

      private:

         policy::canonical::Representation m_info;
         const char* m_name = nullptr;

      };


      using base = reader;

      reader( auto&& node) : m_root{ std::forward< decltype( node)>( node)}, m_stack{ &m_root} {}

      std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* const name)
      {
         if( auto node = append< ::poly::node::array, ::poly::node::nothing>( name))
         {
            if( node->is_nothing())
               return { 0, true};

            auto& array = node->as_array();

            for( auto& item : std::ranges::reverse_view( array))
               m_stack.push_back( &item);
            
            return { array.size(), true};
         }

         return {};
      }

      void container_end( const char*)
      {
         m_stack.pop_back();
      }

      bool composite_start( const char* const name)
      {
         return append< ::poly::node::object, ::poly::node::nothing>( name) != nullptr;
      }

      void composite_end( const char* const name)
      {
         m_stack.pop_back();
      }

      bool read( bool& value, const char* const name)
      {
         return assign< ::poly::node::boolean>( name, value);
      }

      bool read( std::integral auto& value, const char* const name)
      {
         return assign< ::poly::node::integer>( name, value);
      }
      
      bool read( std::floating_point auto& value, const char* const name)
      {
         return assign< ::poly::node::decimal, ::poly::node::integer>( name, value);
      }
      
      bool read( std::string& value, const char* const name)
      {
         if( assign< ::poly::node::string>( name, value))
            return value = transcode::utf8::string::decode( std::move( value)), true;
         return false;
      }
      
      bool read( char& value, const char* const name)
      {
         std::string data;
         if( read( data, name))
            return value = *data.data(), true;
         return false;
      }
      
      bool read( std::u8string& value, const char* const name)
      {
         std::string data;
         if( read( data, name))
            return value = transcode::utf8::cast( std::move( data)), true;
         return false;
      }

      bool read( platform::binary::type& value, const char* const name)
      {
         std::string data;
         if( read( data, name))
            return value = transcode::base64::decode( std::move( data)), true;

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

      auto canonical() const
      {
         return official{}( m_root);
      }

   protected:

      auto search( const char* const name) -> ::poly::node*
      {
         if( name)
         {
            if( auto node = (*m_stack.back())( name))
               return &(*node);

            return nullptr;
         }

         assert( ! m_stack.empty());
         auto result = m_stack.back();
         m_stack.pop_back();
         return result;
      }

      template< typename... types>
      auto verify( const char* const name) -> ::poly::node*
      {
         if( auto node = search( name))
         {
            if( (std::holds_alternative< types>( *node) || ...))
               return node;
            
            // treat nothing as non existent and not as an error
            if( ! std::holds_alternative< ::poly::node::nothing>( *node))
               code::raise::error( code::casual::invalid_node, "unexpected type");
         }

         return nullptr;
      }

      template< typename... types>
      auto append( const char* const name) -> ::poly::node*
      {
         if( auto node = verify< types...>( name))
            return m_stack.push_back( node), node;

         return nullptr;
      }

      template< typename... types>
      bool assign( const char* const name, auto& data)
      {
         if( auto node = verify< types...>( name))
         {
            std::visit( [&data]( auto& node)
            {
               if constexpr( (std::same_as< std::remove_cvref_t< decltype( node)>, types> || ...))
                  data = std::move( node);
            }, *node);

            return true;
         }

         return false;
      }
      
   protected:

      ::poly::node m_root;
      std::vector< ::poly::node*> m_stack;
   };


   class writer
   {
   public:

      writer() : m_stack{ &m_root} {}

      platform::size::type container_start( const platform::size::type size, const char* const name)
      {
         complex(name, ::poly::node::array{}).as_array().reserve( size);

         return size;
      }
      
      void container_end( const char* const name)
      {
         m_stack.pop_back();
      }

      void composite_start( const char* const name)
      {
         complex( name, ::poly::node::object{});
      }

      void composite_end( const char* const name)
      {
         m_stack.pop_back();
      }

      void write( auto&& data, const char* const name)
      {
         trivial( name, std::forward< decltype( data)>( data));
      }

      void write( char data, const char* const name)
      {
         trivial( name, transcode::utf8::string::encode( std::string{ data}));
      }

      void write( std::string_view data, const char* const name)
      {
         trivial( name, transcode::utf8::string::encode( data));
      }

      void write( std::u8string_view data, const char* const name)
      {
         trivial( name, std::string{ transcode::utf8::cast( data)});
      }

      void write( const std::string& data, const char* const name)
      {
         trivial( name, transcode::utf8::string::encode( data));
      }

      void write( const std::u8string& data, const char* const name)
      {
         trivial( name, std::string{ transcode::utf8::cast( data)});
      }

      void write( std::span< const std::byte> data, const char* const name)
      {
         trivial( name, transcode::base64::encode( data));
      }

      void write( binary::span::Fixed< const std::byte> data, const char* const name)
      {
         trivial( name, transcode::base64::encode( data));
      }

   protected:

      auto conduct( const char* const name) -> ::poly::node&
      {
         if( name)
            return (*m_stack.back())[ name];

         if( m_stack.back()->is_array())
            return m_stack.back()->as_array().emplace_back();

         return *m_stack.back();
      }
   
      void trivial( const char* const name, auto&& data)
      {
         auto& slot = conduct( name);
         slot = std::forward< decltype( data)>( data);
      }

      auto complex( const char* const name, auto&& data) -> ::poly::node&
      {
         auto& slot = conduct( name);
         slot = std::forward< decltype( data)>( data);
         return m_stack.push_back( &slot), slot;
      }

   protected:

      ::poly::node m_root;
      std::vector< ::poly::node*> m_stack;

   };

} // casual::common::serialize::detail::poly
