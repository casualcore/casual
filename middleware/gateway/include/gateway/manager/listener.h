//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_LISTENER_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_LISTENER_H_

#include "common/communication/tcp.h"

#include "gateway/message.h"


#include <thread>
#include <ostream>

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         struct Listener
         {
            enum class State
            {
               absent,
               spawned,
               running,
               signaled,
               exit,
               error,
            };

            Listener( common::communication::tcp::Address address);
            ~Listener();

            Listener( Listener&&) noexcept;
            Listener& operator = ( Listener&&) noexcept;


            void start();
            void shutdown();

            void event( const message::manager::listener::Event& event);

            State state() const;
            const common::communication::tcp::Address& address() const;

            bool running() const;

            const common::Uuid& correlation() const { return m_correlation;}

            friend bool operator < ( const Listener& lhs, const common::Uuid& rhs);
            inline friend bool operator < ( const common::Uuid& lhs, const Listener& rhs) { return rhs < lhs;}
            friend bool operator == ( const Listener& lhs, const common::Uuid& rhs);

            friend std::ostream& operator << ( std::ostream& out, const Listener::State& value);
            friend std::ostream& operator << ( std::ostream& out, const Listener& value);

         private:



            common::Uuid m_correlation = common::uuid::make();
            State m_state = State::absent;
            common::communication::tcp::Address m_address;
            std::thread m_thread;
         };

      } // manager

   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_LISTENER_H_
