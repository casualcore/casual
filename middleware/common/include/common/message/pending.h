//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/marshal/binary.h"
#include "common/process.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace pending
         {
            //! A pending message, that will be sent later.
            struct Message
            {
               using destination_type = common::process::Handle;
               using destinations_type = std::vector< destination_type>;
               
               inline Message( communication::message::Complete&& complete, destinations_type destinations) 
                  : destinations( std::move( destinations)), complete( std::move( complete)) {}

               template< typename M>
               Message( M&& message, destinations_type destinations)
                  : Message{ marshal::complete( std::forward< M>( message)), std::move( destinations)} {}

               template< typename M>
               Message( M&& message, destination_type destination)
                  : Message{ marshal::complete( std::forward< M>( message)), destinations_type{ destination}} {}

               
               Message() = default;

               Message( Message&&) noexcept = default;
               Message& operator = ( Message&&) noexcept = default;

               auto type() const noexcept { return complete.type;}

               bool sent() const;
               explicit operator bool () const;

               void remove( strong::ipc::id ipc);
               void remove( strong::process::id pid);

               destinations_type destinations;
               communication::message::Complete complete;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & destinations;
                  archive & complete;
               })

               friend std::ostream& operator << ( std::ostream& out, const Message& value);
            };


            namespace non
            {
               namespace blocking
               {
                  //! return true if message is sent, or destination is removed
                  bool send( Message& message, const communication::error::type& handler = nullptr);

               } // blocking
            } // non

         } // pending
      } // message
   } // common
} // casual


