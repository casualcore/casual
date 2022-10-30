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
   namespace cli
   {
      namespace pipe
      {
         namespace terminal
         {
            //! @returns true if stdout is 'connected' to a terminal
            bool out();
            //! @returns true if stdin is 'connected' to a terminal
            bool in();
         } // terminal

         namespace detail
         {
            namespace create
            {
               template< typename... Ts>
               auto handler( Ts&&... ts)
               {
                  return common::message::dispatch::handler( 
                     common::communication::stream::inbound::Device{ std::cin}, std::forward< Ts>( ts)...);

               }
            } // create
         } // detail

         namespace forward
         {
            namespace standard
            {
               // forwards standard in to standard out.
               void in();
            } // standard

            template< typename M>
            void message( const M& message)
            {
               if( ! message.execution)
                  message.execution = common::execution::id();

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
               auto message()
               {
                  return []( const M& message)
                  {
                     forward::message( message);
                  };
               }

               // default forwards for messages that caller don't want to handle, just forward.
               auto defaults()
               {
                  return detail::create::handler( 
                     handle::message< message::transaction::Directive>(),
                     handle::message< message::Payload>(),
                     handle::message< message::queue::Message>(),
                     handle::message< message::queue::message::ID>(),
                     handle::message< message::transaction::Propagate>());
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
                  return detail::create::handler( 
                     handle::message< message::transaction::Directive>(),
                     handle::message< message::Payload>(),
                     handle::message< message::queue::message::ID>(),
                     handle::message< message::transaction::Propagate>());
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

         void done();

         namespace transaction
         {
            struct Association
            {
               using handler_type = decltype( detail::create::handler());

               void operator() ( common::transaction::ID& trid);

               template< typename M>
               auto operator() ( M& message) -> decltype( std::declval< Association&>()( message.transaction.trid))
               {
                  (*this)( message.transaction.trid);
               }

               template< typename M>
               auto operator() ( M& message) -> decltype( std::declval< Association&>()( message.trid))
               {
                  (*this)( message.trid);
               }

               inline explicit operator bool() const { return static_cast< bool>( directive);}

               cli::message::transaction::Directive directive;
               std::vector< common::transaction::ID> associated;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( directive);
                  CASUAL_SERIALIZE( associated);
               )
               
            };
            
            namespace association
            {
               inline auto single()
               {
                  return Association{ cli::message::transaction::directive::single(), {}};
               }

               inline auto compound()
               {
                  return Association{ cli::message::transaction::directive::compound(), {}};
               }
            } // association

            namespace handle
            {
               common::function< void(cli::message::transaction::Directive&)> directive( Association& associator);
               
            } // handle
         } // transaction

         
         namespace handle
         {
            //! @returns a handler that sets `done` to true when done-message is consumed
            auto done( bool& done)
            {
               return [&done]( const cli::message::pipe::Done& message)
               {
                  common::log::line( verbose::log, "done: ", message);
                  done = true;
               };
            }

            namespace detail
            {
               template< typename M, typename A>
               auto associate( A& associate)
               {
                  return [&associate]( M& message)
                  {
                     Trace trace{ "cli::pipe::handle::associate"};
                     common::log::line( verbose::log, "message: ", message);
                     associate( message);
                  };
               }
            } // detail


            template< typename A>
            auto associate( A& associate)
            {
               return pipe::detail::create::handler( 
                  detail::associate< message::Payload>( associate),
                  detail::associate< message::queue::message::ID>( associate));
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


      } // pipe

   } // cli
} // casual