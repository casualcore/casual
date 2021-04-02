//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/common.h"

#include "common/communication/tcp.h"
#include "common/communication/select.h"
#include "common/algorithm.h"
#include "common/signal/timer.h"

#include <chrono>

namespace casual
{
   namespace gateway::group::tcp
   {

      namespace connection
      {
         struct Metric
         {

         };
      } // connection

      struct Connection
      {
         inline explicit Connection( common::communication::tcp::Duplex&& device)
            : m_device{ std::move( device)} {}

         using complete_type = common::communication::tcp::message::Complete;

         auto descriptor() const noexcept { return m_device.connector().descriptor();}

         //! tries to send as much of unsent as possible, if any.
         //! @returns true if there are unsent left.
         template< typename State>
         void unsent( State& state)
         {
            Trace trace{ "gateway::group::tcp::Connection::unsent"};

            auto keep = common::algorithm::find_if( m_unsent, [&]( auto& complete)
            {
               complete = common::communication::device::non::blocking::send( 
                  m_device,
                  std::move( complete));

               // return true if the message is not fully sent, hence, we found one and stop the 
               // traversal.
               return ! complete;
            });
            
            // if we got stuff left, we trim unsent. If not, we're clean
            // and unset the select trigger.
            if( keep)
               common::algorithm::trim( m_unsent, keep);
            else
            {
               m_unsent.clear();
               state.directive.write.remove( descriptor());
            }
         }
         
         template< typename State, typename M>
         auto send( State& state, M&& message)
         {
            Trace trace{ "gateway::group::tcp::Connection::send"};

            if( m_unsent.empty())
            {
               auto complete = common::communication::device::non::blocking::send( 
                  m_device,
                  std::forward< M>( message));

               if( complete)
                  return complete.correlation();

               m_unsent.push_back( std::move( complete));
               state.directive.write.add( descriptor());
               return m_unsent.back().correlation();
            }

            // we just push it to unsent and wait for 'select' to trigger 'unsent'
            m_unsent.push_back( common::serialize::native::complete< complete_type>( std::forward< M>( message)));
            return m_unsent.back().correlation();
         }


         auto next()
         {
            return common::communication::device::blocking::next( m_device);
         }

         inline friend bool operator == ( const Connection& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor() == rhs;}
         
         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE_NAME( m_device, "device");
            CASUAL_SERIALIZE_NAME( m_unsent, "unsent");
            CASUAL_SERIALIZE_NAME( m_unsent.capacity(), "unsent.capacity");
         )

      private:
         std::vector< complete_type> m_unsent;
         common::communication::tcp::Duplex m_device;
      };

     
      //! Tries to connect the provided connections, if connect success, give
      //! the socket and the connection to `functor`.
      //! used by outbound and reverse inbound 
      template< typename C, typename F>
      void connect( C& connections, F&& functor)
      {
         Trace trace{ "gateway::group::tcp::connect"};

         auto connected = [&functor]( auto& connection)
         {
            try
            {
               ++connection.metric.attempts;
               if( auto socket = common::communication::tcp::non::blocking::connect( connection.configuration.address))
               {
                  common::log::line( common::log::category::information, 
                     "connection established local: ", common::communication::tcp::socket::address::host( socket.descriptor()),
                     " - peer: ", common::communication::tcp::socket::address::peer( socket.descriptor()));

                  socket.set( common::communication::socket::option::File::no_block);
                  // let the functor do what it needs to.
                  functor( std::move( socket), std::move( connection));

                  return true;
               }
               return false;
            }
            catch( ...)
            {
               auto error = common::exception::capture();
               common::log::line( common::log::category::error, error, " connect severely failed for address: '", connection.configuration.address, "' - action: discard connection");
               return true;
            }
         };

         common::algorithm::trim( connections, common::algorithm::remove_if( connections, connected));

         // check if we need to set a timeout to keep trying to connect

         auto min_attempts = []( auto& l, auto& r){ return l.metric.attempts < r.metric.attempts;};

         if( auto min = common::algorithm::min( connections, min_attempts))
         {
            if( min->metric.attempts < 100)
               common::signal::timer::set( std::chrono::milliseconds{ 10});
            else
               common::signal::timer::set( std::chrono::seconds{ 3});
         }

         common::log::line( verbose::log, "connections: ", connections);
      }

      template< typename State, typename M, typename L>
      common::Uuid send( State& state, common::strong::file::descriptor::id descriptor, M&& message, L&& lost)
      {
         try
         {
            if( auto connection = state.external.connection( descriptor))
            {
               return connection->send( state, std::forward< M>( message));
            }
            else
            {
               common::log::line( common::log::category::error, common::code::casual::internal_correlation, " failed to correlate descriptor: ", descriptor);
               common::log::line( common::log::category::verbose::error, "state: ", state);
            }
         }
         catch( ...)
         {
            if( common::exception::capture().code() != common::code::casual::communication_unavailable)
               throw;
            
            lost( state, descriptor);
                  
         }
         return {};
      }

      namespace pending::send
      {
         template< typename State>
         auto dispatch( State& state)
         {
            return [&state]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::write)
            {
               Trace trace{ "gateway::group::tcp::pending::send::dispatch"};
               common::log::line( verbose::log, "descriptor: ", descriptor);

               if( auto connection = state.external.connection( descriptor))
               {
                  connection->unsent( state);
                  return true;
               }

               return false;

            };
         }
      } // pending::send
      
   } // gateway::tcp
} // casual