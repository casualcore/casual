//!
//! type.h
//!
//! Created on: Sep 17, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_BUFFER_TYPE_H_
#define CASUAL_COMMON_BUFFER_TYPE_H_

#include "common/platform.h"



#include <string>
#include <ostream>

namespace casual
{
   namespace common
   {
      namespace buffer
      {
         struct Type
         {
            Type() = default;
            Type( std::string type, std::string subtype) : type( std::move( type)), subtype( std::move( subtype)) {}
            Type( const char* type, const char* subtype) : type( type ? type : ""), subtype( subtype ? subtype : "") {}

            std::string type;
            std::string subtype;

            template< typename A>
            void marshal( A& archive)
            {
               archive & type;
               archive & subtype;
            }

            friend bool operator < ( const Type& lhs, const Type& rhs)
            {
               if( lhs.type < rhs.type)
                  return true;
               if( rhs.type < lhs.type)
                  return false;
               return lhs.subtype < rhs.subtype;
            }

            friend bool operator == ( const Type& lhs, const Type& rhs)
            {
               return lhs.type == rhs.type && lhs.subtype == rhs.subtype;
            }

            friend bool operator != ( const Type& lhs, const Type& rhs)
            {
               return ! ( lhs == rhs);
            }

            friend std::ostream& operator << ( std::ostream& out, const Type& value)
            {
               return out << "{type: " << value.type << " subtype: " << value.subtype << "}";
            }
         };

         struct Payload
         {
            Payload() = default;

            Payload( nullptr_t) : type{ "NULL", ""} {}

            Payload( Type type, platform::binary_type buffer)
             : type( std::move( type)), memory( std::move( buffer)) {}

            Payload( Type type, platform::binary_type::size_type size)
             : type( std::move( type)), memory( size) {}

            //!
            //! g++ does not generate noexecpt move ctor/assignment
            //! @{
            Payload( Payload&& rhs) noexcept
            {
               type = std::move( rhs.type);
               memory = std::move( rhs.memory);
            }
            Payload& operator = ( Payload&& rhs) noexcept
            {
               type = std::move( rhs.type);
               memory = std::move( rhs.memory);
               return *this;
            }
            //! @}


            Payload( const Payload&)  = delete;
            Payload& operator = ( const Payload&) = delete;

            Type type;
            platform::binary_type memory;

            template< typename A>
            void marshal( A& archive)
            {
               archive & type;
               archive & memory;
            }
         };

         struct Buffer
         {
            Buffer( Payload payload) : payload( std::move( payload)) {}

            Buffer( Type type, platform::binary_type::size_type size)
             : payload( std::move( type), size) {}

            Payload payload;
         };

      } // buffer
   } // common




} // casual

#endif // TYPE_H_
