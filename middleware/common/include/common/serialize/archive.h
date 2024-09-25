//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/archive/property.h"
#include "common/serialize/archive/consume.h"
#include "common/serialize/value.h"
#include "casual/platform.h"
#include "common/binary/span.h"

#include <utility>
#include <memory>

namespace casual
{
   namespace common::serialize
   {
      struct Reader
      {
         ~Reader();

         Reader( Reader&&) noexcept;
         Reader& operator = ( Reader&&) noexcept;

         template< typename Protocol, typename... Ts>
         static Reader emplace( Ts&&... ts) { return { std::make_unique< model< Protocol>>( std::forward< Ts>( ts)...)};}

         inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) { return m_protocol->container_start( size, name);}
         inline void container_end( const char* name) { m_protocol->container_end( name);}

         inline bool composite_start( const char* name) { return m_protocol->composite_start( name);}
         inline void composite_end(  const char* name) { m_protocol->composite_end(  name);}

         template< concepts::serialize::archive::native::read T>
         auto read( T&& value, const char* name)
         {
            return m_protocol->read( value, name);
         }

         //! serialize raw data, no 'size' will be serialized, hence caller has to take care
         //! of this if needed.
         //inline bool read( binary::span::Fixed< std::byte> value, const char* name) { return m_protocol->read( value, name);}

         //! Validates the 'consumed' archive, if the implementation has a validate member function.
         //! It throws if there are information in the source that is not consumed by the object-model
         inline void validate() { m_protocol->validate();}

         //! non static non constexpr indicate that this is a 'dynamic' archive
         inline auto archive_properties() const { return m_properties;};


         template< typename V>
         [[maybe_unused]] Reader& operator >> ( V&& value)
         {
            serialize::value::read( *this, std::forward< V>( value), nullptr);
            return *this;
         }

      private:
         struct Concept
         {
            virtual ~Concept() = default;

            virtual std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) = 0;
            virtual void container_end( const char* name) = 0;

            virtual bool composite_start( const char* name) = 0;
            virtual void composite_end(  const char* name) = 0;

            virtual bool read( bool& value, const char* name) = 0;
            virtual bool read( char& value, const char* name) = 0;
            virtual bool read( short& value, const char* name) = 0;
            virtual bool read( int& value, const char* name) = 0;
            virtual bool read( long& value, const char* name) = 0;
            virtual bool read( long long& value, const char* name) = 0;
            virtual bool read( float& value, const char* name) = 0;
            virtual bool read( double& value, const char* name) = 0;
            virtual bool read( std::string& value, const char* name) = 0;
            virtual bool read( std::u8string& value, const char* name) = 0;
            virtual bool read( platform::binary::type& value, const char* name) = 0;
            //! @todo: Change to std::span with C++20
            virtual bool read( binary::span::Fixed< std::byte> value, const char* name) = 0;

            virtual void validate() = 0;
         };

         template< typename P>
         struct model : Concept
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
            bool read( int& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( long& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( long long& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( float& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( double& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( std::string& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( std::u8string& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( platform::binary::type& value, const char* name) override { return m_protocol.read( value, name);}
            bool read( binary::span::Fixed< std::byte> value, const char* name) override { return m_protocol.read( value, name);}


            void validate() override { validate( m_protocol, traits::priority::tag< 1>{});}

         private:

            template< typename T>
            static auto validate( T& protocol, traits::priority::tag< 1>)
               -> decltype( void( protocol.validate()))
            {
               protocol.validate();
            }
            template< typename T>
            static void validate( T& protocol, traits::priority::tag< 0>)
            {}

            protocol_type m_protocol;
         };



         template< typename Protocol>
         Reader( std::unique_ptr< model< Protocol>>&& model)
            : m_protocol( std::move( model)), m_properties{ Protocol::archive_properties()} {}
         
         std::unique_ptr< Concept> m_protocol;
         archive::Property m_properties;
      };

      static_assert( archive::is::dynamic< Reader>);


      struct Writer
      {
         ~Writer();

         Writer( Writer&&) noexcept;
         Writer& operator = ( Writer&&) noexcept;

         template< typename Protocol, typename... Ts>
         static Writer emplace( Ts&&... ts) { return { std::make_unique< model< Protocol>>( std::forward< Ts>( ts)...)};}

         inline void container_start( platform::size::type size, const char* name) { m_protocol->container_start( size, name);}
         inline void container_end( const char* name) { m_protocol->container_end( name);}

         inline void composite_start( const char* name) { m_protocol->composite_start( name);}
         inline void composite_end(  const char* name) { m_protocol->composite_end( name);}


         //! restricted write, so we don't consume convertible types by mistake
         //! binary types, such as char[16] that easily converts to const std::string& 
         template< concepts::serialize::archive::native::write T>
         auto write( T&& value, const char* name)
         {
            m_protocol->write( value, name);
         }

         //! consumes the writer
         //! @{
         inline void consume( platform::binary::type& destination) { m_protocol->consume( destination);};
         inline void consume( std::ostream& destination) { m_protocol->consume( destination);};
         inline void consume( std::string& destination) { m_protocol->consume( destination);};

         template< typename T>
         [[nodiscard]] auto consume()
         {
            T destination;
            consume( destination);
            return destination;
         }
         //! @}

         //! non static non constexpr indicate that this is a 'dynamic' archive
         inline auto archive_properties() const { return m_properties;};

         template< typename V>
         [[maybe_unused]] Writer& operator << ( V&& value)
         {
            serialize::value::write( *this, std::forward< V>( value), nullptr);
            return *this;
         }

      private:

         struct Concept
         {
            virtual ~Concept() = default;

            virtual void container_start( platform::size::type size, const char* name) = 0;
            virtual void container_end( const char* name) = 0;

            virtual void composite_start( const char* name) = 0;
            virtual void composite_end(  const char* name) = 0;

            virtual void write( bool value, const char* name) = 0;
            virtual void write( char value, const char* name) = 0;
            virtual void write( short value, const char* name) = 0;
            virtual void write( int value, const char* name) = 0;
            virtual void write( long value, const char* name) = 0;
            virtual void write( long long value, const char* name) = 0;
            virtual void write( float value, const char* name) = 0;
            virtual void write( double value, const char* name) = 0;
            virtual void write( std::string_view value, const char* name) = 0;
            virtual void write( std::u8string_view value, const char* name) = 0;
            virtual void write( std::span< const std::byte> value, const char* name) = 0;
            virtual void write( binary::span::Fixed< const std::byte> value, const char* name) = 0;

            virtual void consume( platform::binary::type& destination) = 0;
            virtual void consume( std::ostream& destination) = 0;
            virtual void consume( std::string& destination) = 0;
         };

         template< typename protocol_type>
         struct model : Concept
         {
            template< typename... Ts>
            model( Ts&&... ts) : m_protocol( std::forward< Ts>( ts)...) {}

            void container_start( platform::size::type size, const char* name) override { m_protocol.container_start( size, name);}
            void container_end( const char* name) override { m_protocol.container_end( name);}

            void composite_start( const char* name) override { m_protocol.composite_start( name);}
            void composite_end(  const char* name) override { m_protocol.composite_end(  name);}

            void write( bool value, const char* name) override { m_protocol.write( value, name);}
            void write( char value, const char* name) override { m_protocol.write( value, name);}
            void write( short value, const char* name) override { m_protocol.write( value, name);}
            void write( int value, const char* name) override { m_protocol.write( value, name);}
            void write( long value, const char* name) override { m_protocol.write( value, name);}
            void write( long long value, const char* name) override { m_protocol.write( value, name);}
            void write( float value, const char* name) override { m_protocol.write( value, name);}
            void write( double value, const char* name) override { m_protocol.write( value, name);}
            void write( std::string_view value, const char* name) override { m_protocol.write( value, name);}
            void write( std::u8string_view value, const char* name) override { m_protocol.write( value, name);}
            void write( std::span< const std::byte> value, const char* name) override { m_protocol.write( value, name);}
            void write( binary::span::Fixed< const std::byte> value, const char* name) override { m_protocol.write( value, name);}

            void consume( platform::binary::type& destination) override { serialize::writer::consume( m_protocol, destination);};
            void consume( std::ostream& destination) override { serialize::writer::consume( m_protocol, destination);};
            void consume( std::string& destination) override { serialize::writer::consume( m_protocol, destination);};

         private:
            protocol_type m_protocol;
         };

         template< typename Protocol>
         Writer( std::unique_ptr< model< Protocol>>&& model) 
            : m_protocol( std::move( model)), m_properties{ Protocol::archive_properties()} {}

         std::unique_ptr< Concept> m_protocol;
         archive::Property m_properties;
      };

      static_assert( archive::is::dynamic< Writer>);

   } // common::serialize
} // casual


