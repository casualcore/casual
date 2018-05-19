//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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


