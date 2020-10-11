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
                  template< typename... Args>
                  basic_request( common::buffer::payload::Send buffer, Args&&... args)
                        : Base( std::forward< Args>( args)...), buffer( std::move( buffer))
                  {
                  }

                  common::buffer::payload::Send buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     Base::serialize( archive);
                     CASUAL_SERIALIZE( buffer);
                  })
               };

            } // caller

            namespace callee
            {
               template< typename Base>
               struct basic_request : Base
               {
                  using Base::Base;

                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     Base::serialize( archive);
                     CASUAL_SERIALIZE( buffer);
                  })
               };

            } // callee
         } // buffer
      } // message
   } // common
} // casual


