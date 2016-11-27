//!
//! casual
//!

#ifndef CASUAL_COMMON_BUFFER_TYPE_H_
#define CASUAL_COMMON_BUFFER_TYPE_H_

#include "common/platform.h"

#include "common/marshal/marshal.h"
#include "common/algorithm.h"


#include <string>
#include <ostream>

// nullptr_t
#include <cstddef>

namespace casual
{
   namespace common
   {
      namespace buffer
      {


         namespace type
         {
            const std::string& x_octet();
            const std::string& binary();
            const std::string& json();
            const std::string& yaml();
            const std::string& xml();
            const std::string& ini();

            std::string combine( const char* type, const char* subtype = nullptr);

            inline auto dismantle( const std::string& type) -> decltype( range::split( type, '/'))
            {
               return range::split( type, '/');
            }

         } // type



         struct Payload
         {
            Payload();
            Payload( std::nullptr_t);
            Payload( std::string type, platform::binary_type buffer);
            Payload( std::string type, platform::binary_type::size_type size);

            //!
            //! g++ does not generate noexecpt move ctor/assignment
            //! @{
            Payload( Payload&& rhs) noexcept;
            Payload& operator = ( Payload&& rhs) noexcept;
            //! @}


            Payload( const Payload&);
            Payload& operator = ( const Payload&);

            bool null() const;

            std::string type;
            platform::binary_type memory;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & type;
               archive & memory;
            })

            friend std::ostream& operator << ( std::ostream& out, const Payload& value);
         };

         namespace payload
         {
            struct Send
            {
               Send( const Payload& payload, platform::binary_size_type transport, platform::binary_size_type reserved)
                  :  transport( transport), reserved( reserved), m_payload( &payload) {}

               Send( const Payload& payload)
                  : m_payload( &payload) {}


               inline const Payload& payload() const { return *m_payload;};
               platform::binary_size_type transport = 0;
               platform::binary_size_type reserved = 0;



               template< typename A>
               void marshal( A& archive) const
               {
                  archive << payload().type;
                  archive << transport;
                  archive.append( std::begin( payload().memory), std::begin( payload().memory) + transport);
               }

               friend std::ostream& operator << ( std::ostream& out, const Send& value);

            private:
               // gcc 4.9.4 requires Payload to be defined, switch to pointer untill we can use a better compiler
               //std::reference_wrapper< const Payload> m_payload;
               const Payload* m_payload;
            };

         } // payload

         struct Buffer
         {
            Buffer( Payload payload);
            Buffer( std::string type, platform::binary_type::size_type size);

            Buffer( Buffer&&) noexcept;
            Buffer& operator = ( Buffer&&) noexcept;

            Buffer( const Buffer&) = delete;
            Buffer& operator = ( const Buffer&) = delete;

            platform::binary_type::size_type transport( platform::binary_type::size_type user_size) const;

            platform::binary_type::size_type reserved() const;

            Payload payload;
         };

      } // buffer
   } // common




} // casual

#endif // TYPE_H_
