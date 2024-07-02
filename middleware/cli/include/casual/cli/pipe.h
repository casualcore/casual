//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/cli/message.h"
#include "casual/cli/common.h"

namespace casual
{
   namespace cli::pipe
   {
      namespace terminal
      {
         //! @returns true if stdout is 'connected' to a terminal
         bool out();
         //! @returns true if stdin is 'connected' to a terminal
         bool in();
      } // terminal

      namespace forward
      {
         namespace standard
         {
            // forwards standard in to standard out.
            void in();
         } // standard

         namespace human
         {
            template< typename M>
            void message( const M& message)
            {
               if( ! message.execution)
                  message.execution = common::execution::context::get().id;

               cli::message::to::human< M>::stream( message);
            }
         } // human

         template< typename M>
         void message( const M& message)
         {
            if( ! message.execution)
               message.execution = common::execution::context::get().id;

            if( pipe::terminal::out())
               cli::message::to::human< M>::stream( message);
            else
            {
               common::log::line( verbose::log, "cli::pipe::forward::message: ", message);

               common::communication::stream::outbound::Device out{ std::cout};
               common::communication::device::blocking::send( out, message);
            }
         }

         namespace handle
         {
            template< typename M> 
            auto message( bool human = false)
            {
               return [ human]( const M& message)
               {
                  if( human)
                     forward::human::message( message);
                  else
                     forward::message( message);
               };
            }

            // default forwards for messages that caller don't want to handle, just forward.
            // if `human` is true, human readable will be forced
            auto defaults( bool human = false)
            {
               return message::dispatch::create(
                  handle::message< message::payload::Message>( human),
                  handle::message< message::queue::Message>( human),
                  handle::message< message::queue::message::ID>( human),
                  handle::message< message::transaction::Current>( human));
            }
         } // handle
         
      } // forward

      namespace discard
      {
         template< typename M>
         void message( const M& message)
         {
            common::log::line( verbose::log, "cli::pipe::discard::message: ", message);
         }

         namespace handle
         {
            template< typename M> 
            auto message()
            {
               return []( const M& message)
               {
                  discard::message( message);
               };
            }

            // default discard for messages that caller don't want to handle.
            auto defaults()
            {
               return message::dispatch::create( 
                  handle::message< message::payload::Message>(),
                  handle::message< message::queue::message::ID>(),
                  handle::message< message::transaction::Current>());
            }

         } // handle            
      } // discard

      namespace condition
      {
         template< typename T>
         auto done( T& predicate)
         {
            return common::message::dispatch::condition::done( [&predicate]()
            {
               if( predicate || cli::pipe::terminal::in() || std::cin.peek() == std::istream::traits_type::eof())
               {
                  common::log::line( verbose::log, "cli::pipe::condition::done - is done");
                  return true;
               }
               return false;
            });
         };
      } // condition    

      namespace transaction::handle
      {
         //! @returns a handler for transaction::Current, assign the passed `current` and
         //! forward the message downstream.
         auto current( common::transaction::ID& current)
         {
            return [ &current]( const message::transaction::Current& message)
            {
               // if we already got a trid, we assume there is nested transactions
               // going on, and just pass them through.
               if( ! current)
                  current = message.trid;
               forward::message( message);
            };
         }
         
      } // transaction::handle

      namespace done
      {
         using State = cli::message::pipe::State;

         //! Helps with consuming the done message from upstream
         struct Detector : common::traits::unrelocatable
         {

            //! Will only update state if the new state is more _severe_
            //! since this represent the "total pipe outcome state".
            void state( State state);
            State state() const noexcept;

            //! @returns true if `state()` is not `ok`
            bool pipe_error() const noexcept;

            //! @returns true if done has been consumed from upstream -> upstream is done.
            explicit operator bool() const noexcept;
            void operator () ( const cli::message::pipe::Done& message);

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_state);
               CASUAL_SERIALIZE( m_done);
            )

         private:
            State m_state{};
            bool m_done = false;
         };

         //! A helper to make sure we always send done message downstream.
         struct Scope : Detector
         {
            Scope();

            //! will send a pipe::Done downstream regardless (unless downstream is a tty)
            ~Scope(); 

         private:
            int m_uncaught_count;
         };
         
      } // done


      namespace handle
      {
         //! @return a _dispatch handler_ that have handlers for the messages
         //!  `payload::Message`and `queue::Message`, these uses the same 
         //!  provided `handler`. Useful when one wants to handle both messages
         //!  in a generic way. 
         template< typename H>
         auto payloads( H handler)
         {
            return cli::message::dispatch::compose< 
               cli::message::payload::Message, 
               cli::message::queue::Message>( std::move( handler));
         }

      } // handle

      namespace log
      {
         template< typename... Ts> 
         void error( Ts&&... ts)
         {
            common::log::line( std::cerr, ts...);
            common::log::line( common::log::category::error, ts...);
         }
      } // log

   } // cli::pipe
} // casual