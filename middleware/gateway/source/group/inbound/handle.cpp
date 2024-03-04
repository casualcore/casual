//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/handle.h"
#include "gateway/group/inbound/tcp.h"
#include "gateway/group/inbound/task/create.h"
#include "gateway/group/ipc.h"

#include "gateway/message.h"
#include "gateway/common.h"

#include "domain/discovery/api.h"

#include "common/communication/device.h"
#include "common/communication/instance.h"
#include "common/message/dispatch/handle.h"
#include "common/message/internal.h"
#include "common/event/listen.h"
#include "common/event/send.h"

#include "casual/assert.h"

namespace casual
{
   using namespace common;

   namespace gateway::group::inbound::handle
   {   
      namespace local
      {
         namespace
         {
            namespace internal
            {
               template< typename Message>
               auto basic_forward( State& state)
               {
                  return [ &state]( Message& message, strong::ipc::descriptor::id descriptor)
                  {
                     Trace trace{ "gateway::group::inbound::handle::local::basic_forward"};
                     common::log::line( verbose::log, "forward message: ", message);
                     common::log::line( verbose::log, "descriptor: ", descriptor);

                     tcp::send( state, state.external.partner( descriptor), message);
                  };
               }

               template< typename Message>
               auto basic_task( State& state)
               {
                  return [ &state]( Message& message)
                  {
                     Trace trace{ "gateway::group::inbound::handle::local::basic_task"};
                     common::log::line( verbose::log, "message: ", message);

                     state.tasks( message);

                     common::log::line( verbose::log, "state.tasks: ", state.tasks);
                  };
               }

               namespace service
               {
                  namespace lookup
                  {
                     auto reply = internal::basic_task< common::message::service::lookup::Reply>;

                  } // lookup

                  namespace call
                  {
                     auto reply = internal::basic_task< common::message::service::call::Reply>;
                  } // call

               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto reply = internal::basic_forward< common::message::conversation::connect::Reply>;
                     
                  } // connect

                  auto send( State& state)
                  {
                     return [ &state]( common::message::conversation::callee::Send& message, strong::ipc::descriptor::id descriptor)
                     {
                        Trace trace{ "gateway::group::inbound::handle::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        tcp::send( state, state.external.partner( descriptor), message);

                        // if the sender has terminated the conversation we need to clean the task
                        if( message.duplex == decltype( message.duplex)::terminated)
                           state.tasks.remove( message.correlation);
                     };
                  }
                  
               } // conversation

               namespace queue
               {
                  namespace lookup
                  {
                     auto reply = internal::basic_task< casual::queue::ipc::message::lookup::Reply>;
                  } // lookup

                  namespace dequeue
                  {
                     auto reply = internal::basic_forward< casual::queue::ipc::message::group::dequeue::Reply>;
                  } // dequeue

                  namespace enqueue
                  {
                     auto reply = internal::basic_forward< casual::queue::ipc::message::group::enqueue::Reply>;
                  } // enqueue
               } // queue

               namespace domain
               {
                  namespace discovery
                  {
                     auto reply = basic_forward< casual::domain::message::discovery::Reply>;

                     namespace topology::implicit
                     {
                        auto update( State& state)
                        {
                           return [&state]( const casual::domain::message::discovery::topology::implicit::Update& message, strong::ipc::descriptor::id descriptor)
                           {
                              Trace trace{ "gateway::group::inbound::handle::local::internal::domain::discovery::topology::implicit::update"};
                              common::log::line( verbose::log, "message: ", message);

                              auto tcp = state.external.partner( descriptor);

                              inbound::tcp::send( state, tcp, message);
                           };
                        }

                     } // topology::implicit

                  } // discovery
               } // domain


               namespace transaction
               {
                  namespace resource
                  {
                     namespace prepare
                     {
                        auto reply = internal::basic_task< common::message::transaction::resource::prepare::Reply>;
                     } // prepare
                     namespace commit
                     {
                        //! when we get this reply, we know the transaction is 'done', at least in this domain (and downstream)
                        auto reply = internal::basic_task< common::message::transaction::resource::commit::Reply>;
                     } // commit
                     namespace rollback
                     {
                        //! when we get this reply, we know the transaction is 'done', at least in this domain (and downstream)
                        auto reply = internal::basic_task< common::message::transaction::resource::rollback::Reply>;
                     } // commit
                  } // resource


                  namespace coordinate::inbound
                  {
                     auto reply = internal::basic_task< common::message::transaction::coordinate::inbound::Reply>;
                  } // coordinate::inbound   

               } // transaction

            } // internal

            namespace external
            {
               namespace disconnect
               {
                  auto reply( State& state)
                  {
                     return [ &state]( const gateway::message::domain::disconnect::Reply& message, strong::socket::id descriptor)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::disconnect::reply"};
                        common::log::line( verbose::log, "message: ", message);
                        common::log::line( verbose::log, "descriptor: ", descriptor);

                        if( state.disconnectable( descriptor))
                        {
                           // connection is 'idle', we just 'loose' the connection
                           handle::connection::lost( state, descriptor);

                           // we return false to tell the dispatch not to try consume any more messages on
                           // this connection
                           return false;
                        }
                        else
                        {
                           common::log::line( verbose::log, "state.transaction_cache: ", state.transaction_cache);

                           // connection still got pending stuff to do, we keep track until its 'idle'.
                           state.pending.disconnects.push_back( descriptor);
                        }

                        return true;
                     };
                  }
                  
               } // disconnect

               namespace service
               {
                  namespace call
                  {
                     auto request( State& state)
                     {
                        return [ &state]( common::message::service::call::callee::Request& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::service::call::request"};
                           log::line( verbose::log, "message: ", message);
                           
                           state.tasks.add( task::create::service::call( state, descriptor, std::move( message)));
                        };
                     }
                  } // call
               } // service

               namespace conversation
               {
                  namespace connect
                  {
                     auto request( State& state)
                     {
                        return [&state]( common::message::conversation::connect::callee::Request& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::conversation::connect::request"};
                           log::line( verbose::log, "message: ", message);

                           state.tasks.add( task::create::service::conversation( state, descriptor, std::move( message)));
                        };
                     }
                  } // connect

                  auto disconnect( State& state)
                  {
                     return [&state]( common::message::conversation::Disconnect& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::conversation::disconnect"};
                        common::log::line( verbose::log, "message: ", message);

                        // will end/remove the task
                        state.tasks( message);
                     };
                  }

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        // uses the previous lookup address to pass through the send message.
                        state.tasks( message);
                     };
                  }
                  
               } // conversation


               namespace queue
               {
                  namespace enqueue
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::enqueue::Request& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::queue::enqueue::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           state.tasks.add( task::create::queue::enqueue( state, descriptor, std::move( message)));
                        };
                     }
                  } // enqueue

                  namespace dequeue
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::dequeue::Request& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::queue::dequeue::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           state.tasks.add( task::create::queue::dequeue( state, descriptor, std::move( message)));
                        };
                     }
                  } // dequeue
               } // queue

               namespace domain
               {
                  namespace discovery
                  {
                     auto request( State& state)
                     {
                        return [ &state]( casual::domain::message::discovery::Request& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::domain::discovery::request"};
                           common::log::line( verbose::log, "message: ", message);

                           // Set 'sender' so we get the reply
                           message.process = state.external.process_handle( descriptor);

                           {
                              auto information = casual::assertion( state.external.information( descriptor), "invalid descriptor: ", descriptor);

                              if( information->configuration.discovery == decltype( information->configuration.discovery)::forward)
                                 message.directive = decltype( message.directive)::forward;
                           }

                           casual::domain::discovery::request( message);                    
                        };
                     }
                  } // discovery
               } // domain
         
               namespace transaction
               {
                  namespace resource
                  {
                     template< typename Message>
                     auto basic_request( State& state)
                     {
                        return [&state]( Message& message, strong::socket::id descriptor)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::transaction::basic_request"};
                           common::log::line( verbose::log, "message: ", message);

                           state.tasks.add( task::create::transaction( state, descriptor, std::move( message)));
                        };
                     }

                     namespace prepare
                     {
                        auto request = basic_request< common::message::transaction::resource::prepare::Request>;
                     } // prepare
                     namespace commit
                     {
                        auto request = basic_request< common::message::transaction::resource::commit::Request>;
                     } // commit
                     namespace rollback
                     {
                        auto request = basic_request< common::message::transaction::resource::rollback::Request>;
                     } // rollback
                  } // resource
               } // transaction
            } // external

            namespace management
            {
               namespace domain
               {
                  auto connected( State& state)
                  {
                     return [&state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::inbound::handle:::management::domain::connected"};
                        common::log::line( verbose::log, "message: ", message);

                        auto descriptors = state.external.connected( state.directive, message);

                        auto connection = state.external.find_external( descriptors.tcp);
                        CASUAL_ASSERT( connection);

                        // If the connection is not compatible with implicit::Update, we don't need to register to discovery
                        if( ! message::protocol::compatible< casual::domain::message::discovery::topology::implicit::Update>( connection->protocol()))
                           return;

                        auto information = state.external.information( descriptors.tcp);
                        CASUAL_ASSERT( information);

                        // if the connection is not configured with _forward_, there's no need to register to discovery
                        if( information->configuration.discovery != decltype( information->configuration.discovery)::forward)
                           return;

                        auto internal = state.external.find_internal( descriptors.ipc);
                        CASUAL_ASSERT( internal);

                        // a new connection has been established, we need to register this with discovery.
                        casual::domain::discovery::provider::registration( *internal, casual::domain::discovery::provider::Ability::topology);

                        common::log::line( verbose::log, "state.external: ", state.external);
      
                     };
                  }
                  
               } // domain

               namespace event::transaction
               {
                  auto disassociate( State& state)
                  {
                     return [ &state]( const common::message::event::transaction::Disassociate& message)
                     {
                        Trace trace{ "gateway::inbound::handle:::event::transaction::disassociate"};
                        log::line( verbose::log, "message: ", message);

                        state.transaction_cache.remove(  message.gtrid.range());
                     };
                  }
                  
               } // event::transaction

            } // management

         } // <unnamed>
      } // local

      management_handler management( State& state)
      {
         return management_handler{
            common::event::listener( 
               handle::local::management::event::transaction::disassociate( state)
            ),
            local::management::domain::connected( state)
         };

      }

      internal_handler internal( State& state)
      {
         return internal_handler{

            // lookup
            local::internal::service::lookup::reply( state),
            
            // service
            local::internal::service::call::reply( state),

            // conversation
            local::internal::conversation::connect::reply( state),
            local::internal::conversation::send( state),

            // queue
            local::internal::queue::lookup::reply( state),
            local::internal::queue::dequeue::reply( state),
            local::internal::queue::enqueue::reply( state),

            // domain discovery
            local::internal::domain::discovery::reply( state),
            local::internal::domain::discovery::topology::implicit::update( state),

            // transaction
            local::internal::transaction::resource::prepare::reply( state),
            local::internal::transaction::resource::commit::reply( state),
            local::internal::transaction::resource::rollback::reply( state),
            local::internal::transaction::coordinate::inbound::reply( state),
         };
      }

      external_handler external( State& state)
      {
         return external_handler{
            
            local::external::disconnect::reply( state),

            // service call
            local::external::service::call::request( state),

            // conversation
            local::external::conversation::connect::request( state),
            local::external::conversation::disconnect( state),
            local::external::conversation::send( state),

            // queue
            local::external::queue::enqueue::request( state),
            local::external::queue::dequeue::request( state),

            // discover
            local::external::domain::discovery::request( state),

            // transaction
            local::external::transaction::resource::prepare::request( state),
            local::external::transaction::resource::commit::request( state),
            local::external::transaction::resource::rollback::request( state)
         };
      }


      namespace connection
      {
         message::inbound::connection::Lost lost( State& state, common::strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            static constexpr auto filter_active_trids = []( auto& state, auto trids)
            {
               if( std::empty( trids))
                  return trids;

               common::message::transaction::active::Request request{ process::handle()};
               request.gtrids = algorithm::transform( trids, []( auto& trid){ return transaction::global::ID{ transaction::id::range::global( trid)};});
               auto reply = communication::ipc::call( ipc::manager::transaction(), request);

               algorithm::container::erase_if( trids, [ &reply]( auto& trid)
               {
                  return ! algorithm::contains( reply.gtrids, trid);
               });

               return trids;
            };

            // we need to send an ipc-destroyed event, so other can disassociate stuff with the ipc
            if( auto handle = state.external.process_handle( descriptor))
               common::event::send( common::message::event::ipc::Destroyed{ handle});

            auto extracted = state.extract( descriptor);

            // potentially filter only active trids according to TM.
            // We could have inactive trids still in our state due to we haven't got the
            // event::transaction::Disassociate yet, unlikely but could happen.
            extracted.trids = filter_active_trids( state, std::move( extracted.trids));
            
            // this is only to check if need to log some errors. 
            if( ! extracted.empty())
            {
               if( ! std::empty( extracted.trids))
                  log::error( code::casual::communication_unavailable, "lost connection - address: ", extracted.information.configuration.address, ", domain: ", extracted.information.domain, ", trids: ", extracted.trids);
               else
                  log::error( code::casual::communication_unavailable, "lost connection - address: ", extracted.information.configuration.address, ", domain: ", extracted.information.domain);
               
               log::line( log::category::verbose::error, "extracted: ", extracted);
            }

            // if there are ongoing tasks, let them "fail" their work.
            // Mostly to cancel service lookups and such.
            state.tasks.failed( descriptor);

            return { std::move( extracted.information.configuration), std::move( extracted.information.domain)};
         }

         void disconnect( State& state, common::strong::socket::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            if( auto connection = state.external.find_external( descriptor))
            {
               log::line( verbose::log, "connection: ", *connection);

               if( message::protocol::compatible< message::domain::disconnect::Request>( connection->protocol()))
                  inbound::tcp::send( state, descriptor, message::domain::disconnect::Request{});
               else
                  handle::connection::lost( state, descriptor);
            }
         }
         
      } // connection


      void idle( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::idle"};

         // take care of pending disconnects
         if( ! state.pending.disconnects.empty())
         {
            auto connection_done = [&state]( auto descriptor)
            {
               return state.disconnectable( descriptor);
            };

            auto done = algorithm::container::extract( state.pending.disconnects, algorithm::filter( state.pending.disconnects, connection_done));

            for( auto descriptor : done)
               handle::connection::lost( state, descriptor);
         }
      }

      void shutdown( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::shutdown"};

         state.runlevel = decltype( state.runlevel())::shutdown;

         // try to do a 'soft' disconnect. copy - connection::disconnect mutates external
         for( auto descriptor : state.external.external_descriptors())
            handle::connection::disconnect( state, descriptor);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::abort"};

         state.runlevel = decltype( state.runlevel())::error;

         // 'kill' all sockets, and try to take care of pending stuff. copy - connection::lost mutates external
         for( auto descriptor : state.external.external_descriptors())
            handle::connection::lost( state, descriptor);
      }
      

   } // gateway::group::inbound::handle

} // casual