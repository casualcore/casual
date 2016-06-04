//!
//! pending.h
//!
//! Created on: Oct 12, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MESSAGE_PENDING_H_
#define CASUAL_COMMON_MESSAGE_PENDING_H_


#include "common/marshal/binary.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace pending
         {

            //!
            //! A pending message, that will be sent later.
            //!
            struct Message
            {
               using targets_type = std::vector< platform::ipc::id::type>;
               using target_type = platform::ipc::id::type;

               enum class Targets
               {
                  all,
                  first
               };

               template< typename M>
               Message( M&& message, targets_type targets, Targets task)
                  : targets{ std::move( targets)}, complete{ marshal::complete( std::forward< M>( message))}, task{ task}
               {
               }

               template< typename M>
               Message( M&& message, targets_type targets)
                  : Message( std::forward< M>( message), std::move( targets), Targets::all)
               {
               }

               template< typename M>
               Message( M&& message, target_type target)
                  : Message( std::forward< M>( message), { target}, Targets::first)
               {
               }

               Message( Message&&) = default;
               Message& operator = ( Message&&) = default;

               bool sent() const
               {
                  return targets.empty();
               }

               friend std::ostream& operator << ( std::ostream& out, const Message& value)
               {
                  return out << "{ targets: " << range::make( value.targets) << ", complete: " << value.complete << "}";
               }

               targets_type targets;
               communication::message::Complete complete;
               Targets task;
            };

            namespace policy
            {
               struct consume_unavalibe
               {
                  bool operator () () const
                  {
                     try
                     {
                        throw;
                     }
                     catch( const exception::queue::Unavailable&)
                     {
                        return true;
                     }
                  }
               };

            } // policy

            template< typename P>
            bool send( Message& message, P&& policy, const communication::error::type& handler = nullptr)
            {
               auto send = [&]( platform::ipc::id::type ipc)
                     {
                        try
                        {
                           communication::ipc::outbound::Device device{ ipc};
                           return static_cast< bool>( device.put( message.complete, policy, handler));
                        }
                        catch( const exception::queue::Unavailable&)
                        {
                           return true;
                        }
                     };

               if( message.task == Message::Targets::all)
               {
                  message.targets = range::to_vector(
                        range::remove_if( message.targets, send));
               }
               else
               {
                  if( range::find_if( message.targets, send))
                  {
                     message.targets.clear();
                  }
               }

               return message.sent();
            }

            //!
            //! Tries to send a message to targets.
            //! Depending on the task it will either send to all
            //! or stop when the first is successful.
            //!
            template< typename P>
            struct Send
            {
               using send_policy = P;

               using error_type = communication::error::type;

               Send( send_policy policy, error_type handler) : m_policy( std::move( policy)), m_handler( std::move( handler)) {}
               Send( send_policy policy) : Send( std::move( policy), nullptr) {}
               Send( error_type handler) : Send( send_policy{}, std::move( handler)) {}
               Send() : Send( send_policy{}, nullptr) {}

               //!
               //! @return true if the message has been sent
               //!
               bool operator () ( Message& message)
               {
                  return send( message, m_policy, m_handler);
               }
            private:
               send_policy m_policy;
               error_type m_handler;
            };



            //!
            //! @return a 'pending-sender' that tries to send to targets
            //!
            template< typename P>
            Send< P> sender( P&& policy, communication::error::type handler = nullptr)
            {
               return Send< P>( std::forward< P>( policy), handler);
            }



         } // pending

      } // message
   } // common


} // casual

#endif // PENDING_H_
