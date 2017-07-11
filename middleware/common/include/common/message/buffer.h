//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_BUFFER_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_BUFFER_H_

#include "common/buffer/type.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace buffer
         {
            namespace caller
            {
               template< typename Base>
               struct basic_request : Base
               {
                  using base_type = Base;

                  template< typename... Args>
                  basic_request( common::buffer::payload::Send buffer, Args&&... args)
                        : base_type( std::forward< Args>( args)...), buffer( std::move( buffer))
                  {
                  }

                  common::buffer::payload::Send buffer;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     Base::marshal( archive);
                     archive << buffer;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const basic_request& value)
                  {
                     return out << "{ " << static_cast< const basic_request::base_type&>( value)
                           << ", buffer: " << value.buffer
                           << '}';
                  }
               };

            } // caller

            namespace callee
            {
               template< typename Base>
               struct basic_request : Base
               {
                  using base_type = Base;

                  template< typename... Args>
                  basic_request( Args&&... args)
                        : base_type( std::forward< Args>( args)...)
                  {
                  }

                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     Base::marshal( archive);
                     archive & buffer;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const basic_request& value)
                  {
                     return out << "{ " << static_cast< const basic_request::base_type&>( value)
                           << ", buffer: " << value.buffer
                           << '}';
                  }
               };


            } // callee

         } // buffer
      } // message
   } // common



} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_BUFFER_H_
