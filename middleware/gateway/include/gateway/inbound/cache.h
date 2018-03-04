//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_CACHE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_CACHE_H_

#include "common/communication/message.h"


#include <thread>
#include <condition_variable>
#include <vector>

namespace casual
{
   namespace gateway
   {
      namespace inbound
      {
         using size_type = common::platform::size::type;

         struct Cache
         {
            using complete_type = common::communication::message::Complete;
            using lock_type = std::unique_lock< std::mutex>;

            struct Limit
            {
               Limit();
               Limit( size_type size, size_type messages);

               size_type size = 0;
               size_type messages = 0;
            };

            Cache();
            Cache( Limit limit);
            ~Cache();

            void set( Limit limit) const;

            void terminate() const;

            void add( complete_type&& message) const;
            complete_type get( const common::Uuid& correlation) const;


            //!
            //! Only (?) for unittest
            //! @return
            //!
            Limit size() const;


         private:

            friend std::ostream& operator << ( std::ostream& out, const Cache& value);

            bool vacant( const lock_type&) const;

            enum class State
            {
               vacant,
               limit,
               terminate,
            };

            mutable std::mutex m_mutex;
            mutable Limit m_limit;
            mutable std::condition_variable m_condition;
            mutable std::vector< complete_type> m_messages;
            mutable size_type m_size = 0;
            mutable State m_state = State::vacant;

         };

      } // inbound
   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_INBOUND_CACHE_H_
