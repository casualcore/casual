//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/value.h"
#include "common/serialize/archive.h"
#include "common/stream/customization.h"
#include "common/array.h"

#include <ostream>

namespace casual
{
   namespace common::serialize::line
   {

      namespace detail
      {
         constexpr auto first = "";
         constexpr auto scope = ", ";

         namespace dispatch
         {
            
         } // dispatch
         
      } // detail

      struct Writer
      {
      private:
         template< concepts::arithmetic T>
         auto save( T value, const char* name)
         {
             maybe_name( name) << value;
         }

         void save( bool value, const char* name);
         void save( std::string_view value, const char* name);
         void save( std::u8string_view value, const char* name);
         void save( std::span< const std::byte> value, const char* name);

         std::ostream& maybe_name( const char* name);

         void begin_scope();
         void in_scope();

         std::ostringstream m_stream;
         const char* m_prefix = detail::first;

         template< concepts::serialize::archive::native::write T>
         auto dispatch( T&& value, const char* name, common::traits::priority::tag< 1>) 
         {
            save( std::forward< T>( value), name);
         }

         template< typename T>
         auto dispatch( T&& value, const char* name, common::traits::priority::tag< 0>) 
            -> decltype( void( stream::customization::detail::write( this->m_stream, std::forward< T>( value))))
         {
            stream::customization::detail::write( maybe_name( name), std::forward< T>( value));
         }

      public:
         Writer();
         
         constexpr static auto archive_properties() { return common::serialize::archive::Property::named;}

         constexpr static auto keys() 
         {
            using namespace std::string_view_literals; 
            return array::make( "line"sv);
         }

         platform::size::type container_start( const platform::size::type size, const char* name);
         void container_end( const char*);

         void composite_start( const char* name);
         void composite_end( const char*);


         template<typename T>
         auto write( T&& value, const char* name)
            -> decltype( dispatch( std::forward< T>( value), name, common::traits::priority::tag< 1>{}))
         {
            in_scope();
            dispatch( std::forward< T>( value), name, common::traits::priority::tag< 1>{});
         }

         template< typename T>
         auto operator << ( T&& value)
            -> decltype( void( serialize::value::write( *this, std::forward< T>( value), nullptr)), *this)
         {
            serialize::value::write( *this, std::forward< T>( value), nullptr);
            return *this;
         }
       
         void consume( std::ostream& destination);
         std::string consume();


      };

      //! type erased line writer 
      serialize::Writer writer();


   } // common::serialize::line
} // casual




