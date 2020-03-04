//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/archive/type.h"
#include "common/serialize/archive/consume.h"
#include "common/serialize/traits.h"
#include "common/serialize/value.h"
#include "casual/platform.h"
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

            inline constexpr static auto archive_type() { return archive::Type::dynamic_type;}

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

            inline auto type() const { return m_type;};


            template< typename V>
            Reader& operator >> ( V&& value)
            {
               serialize::value::read( *this, std::forward< V>( value), nullptr);
               return *this;
            }

            template< typename V>
            Reader& operator & ( V&& value)
            {
               return Reader::operator >> ( std::forward< V>( value));
            }

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

            template< typename Protocol>
            Reader( std::unique_ptr< model< Protocol>>&& model)
               : m_protocol( std::move( model)), m_type{ traits::archive::dynamic::convert< Protocol>::value} {}
            
            std::unique_ptr< concept> m_protocol;
            archive::dynamic::Type m_type;
         };

         static_assert( traits::archive::type< Reader>::value == archive::Type::dynamic_type, "");


         class Writer
         {
         public:

            inline constexpr static auto archive_type() { return archive::Type::dynamic_type;}

            ~Writer();

            Writer( Writer&&) noexcept;
            Writer& operator = ( Writer&&) noexcept;

            template< typename Protocol, typename... Ts>
            static Writer emplace( Ts&&... ts) { return { std::make_unique< model< Protocol>>( std::forward< Ts>( ts)...)};}

            inline void container_start( platform::size::type size, const char* name) { m_protocol->container_start( size, name);}
            inline void container_end( const char* name) { m_protocol->container_end( name);}

            inline void composite_start( const char* name) { m_protocol->composite_start( name);}
            inline void composite_end(  const char* name) { m_protocol->composite_end( name);}


            //! restricted write, so we don't consume convertable types by mistake
            //! binary types, such as char[16] that easily converts to const std::string& 
            template< typename T>
            auto write( T&& value, const char* name) 
               -> std::enable_if_t< traits::is::archive::write::type< common::traits::remove_cvref_t< T>>::value>
            {
               save( std::forward< T>( value), name);
            }

            //! consumes the writer
            //! @{
            inline void consume( platform::binary::type& destination) { m_protocol->consume( destination);};
            inline void consume( std::ostream& destination) { m_protocol->consume( destination);};
            inline void consume( std::string& destination) { m_protocol->consume( destination);};

            template< typename T>
            auto consume()
            {
               T destination;
               consume( destination);
               return destination;
            }
            //! @}


            inline auto type() const { return m_type;};


            template< typename V>
            Writer& operator << ( V&& value)
            {
               serialize::value::write( *this, std::forward< V>( value), nullptr);
               return *this;
            }

            template< typename V>
            Writer& operator & ( V&& value)
            {
               return Writer::operator << ( std::forward< V>( value));
            }

         private:

            inline void save( bool value, const char* name) { m_protocol->write( value, name);}
            inline void save( char value, const char* name) { m_protocol->write( value, name);}
            inline void save( short value, const char* name) { m_protocol->write( value, name);}
            void save( int value, const char* name);
            inline void save( long value, const char* name) { m_protocol->write( value, name);}
            void save( unsigned long value, const char* name);
            inline void save( long long value, const char* name) { m_protocol->write( value, name);}
            inline void save( float value, const char* name) { m_protocol->write( value, name);}
            inline void save( double value, const char* name) { m_protocol->write( value, name);}
            inline void save( const std::string& value, const char* name) { m_protocol->write( value, name);}
            inline void save( const platform::binary::type& value, const char* name) { m_protocol->write( value, name);}
            
            //! serialize raw data, no 'size' will be serialized, hence caller has to take care
            //! of this if needed.
            inline void save( view::immutable::Binary value, const char* name) { m_protocol->write( value, name);}

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

               virtual void consume( platform::binary::type& destination) = 0;
               virtual void consume( std::ostream& destination) = 0;
               virtual void consume( std::string& destination) = 0;
            };

            template< typename protocol_type>
            struct model : concept
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
               void write( long value, const char* name) override { m_protocol.write( value, name);}
               void write( long long value, const char* name) override { m_protocol.write( value, name);}
               void write( float value, const char* name) override { m_protocol.write( value, name);}
               void write( double value, const char* name) override { m_protocol.write( value, name);}
               void write( const std::string& value, const char* name) override { m_protocol.write( value, name);}
               void write( const platform::binary::type&value, const char* name) override { m_protocol.write( value, name);}
               void write( view::immutable::Binary value, const char* name) override { m_protocol.write( value, name);}

               void consume( platform::binary::type& destination) override { serialize::writer::consume( m_protocol, destination);};
               void consume( std::ostream& destination) override { serialize::writer::consume( m_protocol, destination);};
               void consume( std::string& destination) override { serialize::writer::consume( m_protocol, destination);};

            private:
               protocol_type m_protocol;
            };


            template< typename Protocol>
            Writer( std::unique_ptr< model< Protocol>>&& model) 
               : m_protocol( std::move( model)), m_type{ traits::archive::dynamic::convert< Protocol>::value} {}

            std::unique_ptr< concept> m_protocol;
            archive::dynamic::Type m_type;
         };

         static_assert( traits::archive::type< Writer>::value == archive::Type::dynamic_type, "");


      } // serialize
   } // common
} // casual


