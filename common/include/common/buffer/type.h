//!
//! type.h
//!
//! Created on: Sep 17, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_BUFFER_TYPE_H_
#define CASUAL_COMMON_BUFFER_TYPE_H_

#include "common/platform.h"

#include "common/marshal/marshal.h"



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

         struct Type
         {
            Type();
            Type( std::string type, std::string subtype);
            Type( const char* type, const char* subtype);

            std::string type;
            std::string subtype;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & type;
               archive & subtype;
            })

            friend bool operator < ( const Type& lhs, const Type& rhs);
            friend bool operator == ( const Type& lhs, const Type& rhs);
            friend bool operator != ( const Type& lhs, const Type& rhs);

            friend std::ostream& operator << ( std::ostream& out, const Type& value);
         };

         struct Payload
         {
            Payload();
            Payload( std::nullptr_t);
            Payload( Type type, platform::binary_type buffer);
            Payload( Type type, platform::binary_type::size_type size);

            //!
            //! g++ does not generate noexecpt move ctor/assignment
            //! @{
            Payload( Payload&& rhs) noexcept;
            Payload& operator = ( Payload&& rhs) noexcept;
            //! @}


            Payload( const Payload&);
            Payload& operator = ( const Payload&);

            Type type;
            platform::binary_type memory;

            CASUAL_CONST_CORRECT_MARSHAL(
            {
               archive & type;
               archive & memory;
            })
         };

         namespace payload
         {
            struct Send
            {
               Send( Payload& payload, platform::binary_size_type transport)
                  : payload( payload), transport( transport) {}

               Payload& payload;
               platform::binary_size_type transport;

               template< typename A>
               void marshal( A& archive) const
               {
                  archive << payload.type;
                  archive << transport;
                  archive.append( std::begin( payload.memory), std::begin( payload.memory) + transport);
               }

            };

         } // payload

         struct Buffer
         {
            Buffer( Payload payload);
            Buffer( Type type, platform::binary_type::size_type size);

            Buffer( Buffer&&) noexcept;
            Buffer& operator = ( Buffer&&) noexcept;

            Buffer( const Buffer&) = delete;
            Buffer& operator = ( const Buffer&) = delete;

            platform::binary_type::size_type size( platform::binary_type::size_type user_size) const;

            Payload payload;
         };

      } // buffer
   } // common




} // casual

#endif // TYPE_H_
