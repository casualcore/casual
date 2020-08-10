//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/native/binary.h"
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

               inline Message( communication::message::Complete&& complete, const destination_type& destination) 
                  : destinations{ destination}, complete( std::move( complete)) {}

               template< typename M>
               Message( M&& message, destinations_type destinations)
                  : Message{ serialize::native::complete( std::forward< M>( message)), std::move( destinations)} {}

               template< typename M>
               Message( M&& message, const destination_type& destination)
                  : Message{ serialize::native::complete( std::forward< M>( message)), destination} {}

               
               Message() = default;

               Message( Message&&) noexcept = default;
               Message& operator = ( Message&&) noexcept = default;

               auto type() const noexcept { return complete.type;}

               //! @returns true if the message is sent to all destinations
               //! @{
               bool sent() const;
               explicit operator bool () const;
               //! @}

               void remove( strong::ipc::id ipc);
               void remove( strong::process::id pid);

               destinations_type destinations;
               communication::message::Complete complete;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( destinations);
                  CASUAL_SERIALIZE( complete);
               })
            };


            namespace non
            {
               namespace blocking
               {
                  //! return true if message is sent, or destination is removed
                  bool send( Message& message);

               } // blocking
            } // non

         } // pending
      } // message
   } // common
} // casual


