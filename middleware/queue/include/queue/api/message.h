//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "common/platform.h"
#include "common/uuid.h"

#include <string>

namespace casual
{
   namespace queue
   {
      inline namespace v1  {

      using size_type = common::platform::size::type;

      struct Attributes
      {
         //! Correlation information.
         std::string properties;

         //! reply queue.
         std::string reply;

         //! When the message is available, in absolute time.
         common::platform::time::point::type available = common::platform::time::point::limit::zero();

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            CASUAL_SERIALIZE( properties);
            CASUAL_SERIALIZE( reply);
            CASUAL_SERIALIZE( available);
         })

      };

      struct Selector
      {
         //! If empty -> not used
         //! If not empty -> the first message that gets a match against the regexp is dequeued.
         std::string properties;

         //! If not 'null', the first message that has this particular id is dequeued
         common::Uuid id;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            CASUAL_SERIALIZE( properties);
            CASUAL_SERIALIZE( id);
         })
      };

      struct Payload
      {
         std::string type;
         common::platform::binary::type data;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            CASUAL_SERIALIZE( type);
            CASUAL_SERIALIZE( data);
         })
      };


      template< typename P>
      struct basic_message
      {
         using payload_type = P;
         using id_type = common::Uuid;

         basic_message( id_type id, Attributes attributes, payload_type payload)
            : id( std::move( id)), attributes( std::move( attributes)), payload( std::move( payload)) {}

         basic_message( payload_type payload)
            : payload( std::move( payload)) {}

         basic_message() = default;

         id_type id;
         Attributes attributes;
         payload_type payload;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            CASUAL_SERIALIZE( id);
            CASUAL_SERIALIZE( attributes);
            CASUAL_SERIALIZE( payload);
         })
      };


      using Message = basic_message< Payload>;


      namespace peek
      {
         namespace message
         {
            struct Information
            {
               common::Uuid id;
               common::platform::binary::type trid;
               size_type state;

               Attributes attributes;

               struct
               {
                  std::string type;
                  size_type size;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( type);
                     CASUAL_SERIALIZE( size);
                  })

               } payload;


               size_type redelivered;
               common::platform::time::point::type timestamp;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( trid);

                  CASUAL_SERIALIZE( state);

                  CASUAL_SERIALIZE( attributes);
                  CASUAL_SERIALIZE( payload);

                  CASUAL_SERIALIZE( redelivered);
                  CASUAL_SERIALIZE( timestamp);
               })
            };

         } // message

      } // peek

      namespace xatmi
      {
         struct Payload
         {
            Payload() = default;
            Payload( common::platform::buffer::raw::type buffer, common::platform::buffer::raw::size::type size)
              : buffer( buffer), size( size) {}

            Payload( common::platform::buffer::raw::type buffer)
              : buffer( buffer), size( 0) {}

            common::platform::buffer::raw::type buffer;
            common::platform::buffer::raw::size::type size;
         };

         using Message = basic_message< Payload>;

      } // xatmi

      } // v1
   } // queue

} // casual


