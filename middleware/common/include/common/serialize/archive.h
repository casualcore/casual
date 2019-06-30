//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/named/value.h"
#include "common/serialize/traits.h"
#include "common/serialize/value.h"
#include "common/platform.h"
#include "common/view/binary.h"

#include <utility>
#include <memory>

namespace casual
{
   namespace common
   {
      namespace serialize
      {

         class Reader
         {
         public:

            using need_named = void;

            ~Reader();

            Reader( Reader&&) noexcept;
            Reader& operator = ( Reader&&) noexcept;

            template< typename Protocol, typename... Ts>
            static Reader emplace( Ts&&... ts) { return { std::make_unique< model< Protocol>>( std::forward< Ts>( ts)...)};}

            inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) { return m_protocol->container_start( size, name);}
            inline void container_end( const char* name) { m_protocol->container_end( name);}

            inline bool composite_start( const char* name) { return m_protocol->composite_start( name);}
            inline void composite_end(  const char* name) { m_protocol->composite_end(  name);}

            inline bool read( bool& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( char& value, const char* name){ return m_protocol->read( value, name);}
            inline bool read( short& value, const char* name) { return m_protocol->read( value, name);}
            bool read( int& value, const char* name);
            inline bool read( long& value, const char* name) { return m_protocol->read( value, name);}
            bool read( unsigned long& value, const char* name);
            inline bool read( long long& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( float& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( double& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( std::string& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( platform::binary::type& value, const char* name) { return m_protocol->read( value, name);}

            //! serialize raw data, no 'size' will be serialized, hence caller has to take care
            //! of this if needed.
            inline bool read( view::Binary value, const char* name) { return m_protocol->read( value, name);}

            //! Validates the 'consumed' archive, if the implementation has a validate member function.
            //! It throws if there are information in the source that is not consumed by the object-model
            inline void validate() { m_protocol->validate();}

         private:

            struct concept
            {
               virtual ~concept() = default;

               virtual std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) = 0;
               virtual void container_end( const char* name) = 0;

               virtual bool composite_start( const char* name) = 0;
               virtual void composite_end(  const char* name) = 0;

               virtual bool read( bool& value, const char* name) = 0;
               virtual bool read( char& value, const char* name) = 0;
               virtual bool read( short& value, const char* name) = 0;
               virtual bool read( long& value, const char* name) = 0;
               virtual bool read( long long& value, const char* name) = 0;
               virtual bool read( float& value, const char* name) = 0;
               virtual bool read( double& value, const char* name) = 0;
               virtual bool read( std::string& value, const char* name) = 0;
               virtual bool read( platform::binary::type& value, const char* name) = 0;
               virtual bool read( view::Binary value, const char* name) = 0;

               virtual void validate() = 0;
            };

            template< typename P>
            struct model : concept
            {
               using protocol_type = P;

               template< typename... Ts>
               model( Ts&&... ts) : m_protocol( std::forward< Ts>( ts)...) {}

               std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) override { return m_protocol.container_start( size, name);}
               void container_end( const char* name) override { m_protocol.container_end( name);}

               bool composite_start( const char* name) override { return m_protocol.composite_start( name);}
               void composite_end(  const char* name) override { m_protocol.composite_end(  name);}

               bool read( bool& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( char& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( short& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( long& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( long long& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( float& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( double& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( std::string& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( platform::binary::type& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( view::Binary value, const char* name) override { return m_protocol.read( value, name);}


               void validate() override { selective_validate( m_protocol);}

            private:
               template< typename T>
               using has_validate = decltype( std::declval< T&>().validate());

               template< typename T>
               static auto selective_validate( T& protocol) -> 
                  std::enable_if_t< common::traits::detect::is_detected< has_validate, T>::value>
               {
                  protocol.validate();
               }
               template< typename T>
               static auto selective_validate( T& protocol) -> 
                  std::enable_if_t< ! common::traits::detect::is_detected< has_validate, T>::value>
               {
               }

               protocol_type m_protocol;
            };

            Reader( std::unique_ptr< concept>&& base) : m_protocol( std::move( base)) {}

            std::unique_ptr< concept> m_protocol;

         };

         //! Reader interface
         template< typename V>
         Reader& operator >> ( Reader& archive, V&& value)
         {
            serialize::value::read( archive, std::forward< V>( value), nullptr);
            return archive;
         }

         template< typename V>
         Reader& operator & ( Reader& archive, V&& value)
         {
            return operator >> ( archive, std::forward< V>( value));
         }


         class Writer
         {
         public:

            using need_named = void;

            ~Writer();

            Writer( Writer&&) noexcept;
            Writer& operator = ( Writer&&) noexcept;

            template< typename Protocol, typename... Ts>
            static Writer emplace( Ts&&... ts) { return { std::make_unique< model< Protocol>>( std::forward< Ts>( ts)...)};}

            inline void container_start( platform::size::type size, const char* name) { m_protocol->container_start( size, name);}
            inline void container_end( const char* name) { m_protocol->container_end( name);}

            inline void composite_start( const char* name) { m_protocol->composite_start( name);}
            inline void composite_end(  const char* name) { m_protocol->composite_end(  name);}

            inline void write( bool value, const char* name) { m_protocol->write( value, name);}
            inline void write( char value, const char* name) { m_protocol->write( value, name);}
            inline void write( short value, const char* name) { m_protocol->write( value, name);}
            void write( int value, const char* name);
            inline void write( long value, const char* name) { m_protocol->write( value, name);}
            void write( unsigned long value, const char* name);
            inline void write( long long value, const char* name) { m_protocol->write( value, name);}
            inline void write( float value, const char* name) { m_protocol->write( value, name);}
            inline void write( double value, const char* name) { m_protocol->write( value, name);}
            inline void write( const std::string& value, const char* name) { m_protocol->write( value, name);}
            inline void write( const platform::binary::type& value, const char* name) { m_protocol->write( value, name);}
            
            //! serialize raw data, no 'size' will be serialized, hence caller has to take care
            //! of this if needed.
            inline void write( view::immutable::Binary value, const char* name) { m_protocol->write( value, name);}

            //! Flushes the archive, if the implementation has a flush member function.
            inline void flush() { m_protocol->flush();}

         private:

            struct concept
            {
               virtual ~concept() = default;

               virtual void container_start( platform::size::type size, const char* name) = 0;
               virtual void container_end( const char* name) = 0;

               virtual void composite_start( const char* name) = 0;
               virtual void composite_end(  const char* name) = 0;

               virtual void write( bool value, const char* name) = 0;
               virtual void write( char value, const char* name) = 0;
               virtual void write( short value, const char* name) = 0;
               virtual void write( long value, const char* name) = 0;
               virtual void write( long long value, const char* name) = 0;
               virtual void write( float value, const char* name) = 0;
               virtual void write( double value, const char* name) = 0;
               virtual void write( const std::string& value, const char* name) = 0;
               virtual void write( const platform::binary::type& value, const char* name) = 0;
               virtual void write( view::immutable::Binary value, const char* name) = 0;

               virtual void flush() = 0;
            };

            template< typename P>
            struct model : concept
            {
               using protocol_type = P;

               template< typename... Ts>
               model( Ts&&... ts) : m_protocol( std::forward< Ts>( ts)...) {}

               void container_start( platform::size::type size, const char* name) override { m_protocol.container_start( size, name);}
               void container_end( const char* name) override { m_protocol.container_end( name);}

               void composite_start( const char* name) override { m_protocol.composite_start( name);}
               void composite_end(  const char* name) override { m_protocol.composite_end(  name);}

               void write( bool value, const char* name) override { m_protocol.write( value, name);}
               void write( char value, const char* name) override { m_protocol.write( value, name);}
               void write( short value, const char* name) override { m_protocol.write( value, name);}
               void write( long value, const char* name) override { m_protocol.write( value, name);}
               void write( long long value, const char* name) override { m_protocol.write( value, name);}
               void write( float value, const char* name) override { m_protocol.write( value, name);}
               void write( double value, const char* name) override { m_protocol.write( value, name);}
               void write( const std::string& value, const char* name) override { m_protocol.write( value, name);}
               void write( const platform::binary::type&value, const char* name) override { m_protocol.write( value, name);}
               void write( view::immutable::Binary value, const char* name) override { m_protocol.write( value, name);}

               void flush() override { selective_flush( m_protocol);}

            private:
               template< typename T>
               static auto selective_flush( T& protocol) -> std::enable_if_t< common::traits::has::flush< T>::value>
               {
                  protocol.flush();
               }
               
               template< typename T>
               static auto selective_flush( T& protocol) -> std::enable_if_t< ! common::traits::has::flush< T>::value> {}

               protocol_type m_protocol;
            };

            Writer( std::unique_ptr< concept>&& base) : m_protocol( std::move( base)) {}

            std::unique_ptr< concept> m_protocol;

         };


         template< typename V>
         Writer& operator << ( Writer& archive, V&& value)
         {
            serialize::value::write( archive, std::forward< V>( value), nullptr);
            return archive;
         }

         template< typename NV>
         Writer& operator & ( Writer& archive, NV&& named)
         {
            return operator << ( archive, std::forward< NV>( named));
         }

      } // serialize
   } // common
} // casual


