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
                  return [&state]( Message& message)
                  {
                     Trace trace{ "gateway::group::inbound::handle::local::basic_forward"};
                     common::log::line( verbose::log, "forward message: ", message);

                     tcp::send( state, message);
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
                     auto reply = internal::basic_task< common::message::conversation::connect::Reply>;
                     
                  } // connect

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::internal::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                        {
                           inbound::tcp::send( state, found->descriptor, message);
                           
                           // if the `send` has event that indicate that it will end the conversation - we remove our "route" state
                           // for the connection
                           if( message.code.result != decltype( message.code.result)::absent)
                              state.conversations.erase( std::begin( found));
                        }
                        else
                           common::log::line( common::log::category::error, "(internal) failed to correlate conversation: ", message.correlation);

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
                     auto reply = internal::basic_task< casual::queue::ipc::message::group::dequeue::Reply>;
                  } // dequeue

                  namespace enqueue
                  {
                     auto reply = internal::basic_task< casual::queue::ipc::message::group::enqueue::Reply>;
                  } // enqueue
               } // queue

               namespace domain
               {
                  auto connected( State& state)
                  {
                     return [&state]( const gateway::message::domain::Connected& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::internal::domain::connected"};
                        common::log::line( verbose::log, "message: ", message);

                        state.external.connected( state.directive, message);

                        common::log::line( verbose::log, "state.external: ", state.external);
       
                     };
                  }

                  namespace discovery
                  {
                     auto reply = basic_forward< casual::domain::message::discovery::Reply>;

                     namespace topology::implicit
                     {
                        auto update( State& state)
                        {
                           return [&state]( const casual::domain::message::discovery::topology::implicit::Update& message)
                           {
                              Trace trace{ "gateway::group::inbound::handle::local::internal::domain::discovery::topology::implicit::update"};
                              common::log::line( verbose::log, "message: ", message);

                              auto send_if_compatible = [ &state, &message]( auto descriptor)
                              {
                                 auto connection = state.external.connection( descriptor);
                                 CASUAL_ASSERT( connection);

                                 if( ! message::protocol::compatible( message, connection->protocol()))
                                    return;

                                 auto information = casual::assertion( state.external.information( descriptor), 
                                    "failed to find information for descriptor: ", descriptor);

                                 if( information->configuration.discovery != decltype( information->configuration.discovery)::forward)
                                    return;

                                 // check if domain has seen this message before...
                                 if( algorithm::find( message.domains, information->domain))
                                    return;

                                 inbound::tcp::send( state, descriptor, message);
                              };

                              algorithm::for_each( state.external.descriptors(), send_if_compatible);

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
               template< typename M>
               auto correlate( State& state, const M& message)
               {
                  state.correlations.emplace_back( message.correlation, state.external.last());
                  return state.external.last();
               }

               namespace disconnect
               {
                  auto reply( State& state)
                  {
                     return [&state]( const gateway::message::domain::disconnect::Reply& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::disconnect::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto descriptor = state.consume( message.correlation))
                        {
                           common::log::line( verbose::log, "descriptor: ", descriptor);

                           if( algorithm::find( state.correlations, descriptor) || state.transaction_cache.associated( descriptor))
                           {
                              common::log::line( verbose::log, "state.correlations: ", state.correlations, ", state.transaction_cache: ", state.transaction_cache);

                              // connection still got pending stuff to do, we keep track until its 'idle'.
                              state.pending.disconnects.push_back( descriptor);
                           }
                           else 
                           {
                              // connection is 'idle', we just 'loose' the connection
                              handle::connection::lost( state, descriptor);

                              // we return false to tell the dispatch not to try consume any more messages on
                              // this connection
                              return false;
                           }
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
                        return [ &state]( common::message::service::call::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::service::call::request"};
                           log::line( verbose::log, "message: ", message);
                           
                           auto descriptor = external::correlate( state, message);
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
                        return [&state]( common::message::conversation::connect::callee::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::conversation::connect::request"};
                           log::line( verbose::log, "message: ", message);

                           auto descriptor = external::correlate( state, message);
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

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                        {
                           state.multiplex.send( found->process.ipc, message);

                           // we're done with this connection, regardless of what callee thinks...
                           state.conversations.erase( std::begin( found));
                        }
                     };
                  }

                  auto send( State& state)
                  {
                     return [&state]( common::message::conversation::callee::Send& message)
                     {
                        Trace trace{ "gateway::group::inbound::handle::local::external::conversation::send"};
                        common::log::line( verbose::log, "message: ", message);

                        if( auto found = algorithm::find( state.conversations, message.correlation))
                           state.multiplex.send( found->process.ipc, message);
                        else
                           common::log::error( code::casual::invalid_semantics, "(external) failed to correlate conversation: ", message.correlation);
                     };
                  }
                  
               } // conversation


               namespace queue
               {
                  namespace enqueue
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::enqueue::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::queue::enqueue::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto descriptor = external::correlate( state, message);
                           state.tasks.add( task::create::queue::enqueue( state, descriptor, std::move( message)));
                        };
                     }
                  } // enqueue

                  namespace dequeue
                  {
                     auto request( State& state)
                     {
                        return [&state]( casual::queue::ipc::message::group::dequeue::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::handle::local::external::queue::dequeue::Request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto descriptor = external::correlate( state, message);
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
                        return [&state]( casual::domain::message::discovery::Request& message)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::domain::discovery::request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto descriptor = external::correlate( state, message);

                           // Set 'sender' so we get the reply
                           message.process = common::process::handle();

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
                        return [&state]( Message& message)
                        {
                           Trace trace{ "gateway::inbound::handle::local::external::transaction::basic_request"};
                           common::log::line( verbose::log, "message: ", message);

                           auto descriptor = external::correlate( state, message);
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

         } // <unnamed>
      } // local

      internal_handler internal( State& state)
      {
         casual::domain::discovery::provider::registration( casual::domain::discovery::provider::Ability::topology);

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

            local::internal::domain::connected( state),
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
         message::inbound::connection::Lost lost( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::lost"};
            log::line( verbose::log, "descriptor: ", descriptor);

            auto extracted = state.extract( descriptor);

            if( ! extracted.empty())
            {
               log::line( log::category::error, code::casual::communication_unavailable, " lost connection - address: ", extracted.information.configuration.address, ", domain: ", extracted.information.domain);
               log::line( log::category::verbose::error, "extracted: ", extracted);

               // Let ongoing task associated with the connections conclude their work.
               // Mostly to cancel service lookups and such.
               algorithm::for_each( extracted.correlations, [ &state]( auto& correlations)
               {
                  casual::task::concurrent::message::Conclude message;
                  message.correlation = correlations;

                  state.tasks( message);
               });          
            }

            return { std::move( extracted.information.configuration), std::move( extracted.information.domain)};
         }

         void disconnect( State& state, common::strong::file::descriptor::id descriptor)
         {
            Trace trace{ "gateway::group::inbound::handle::connection::disconnect"};
            log::line( verbose::log, "descriptor: ", descriptor);

            if( auto connection = state.external.connection( descriptor))
            {
               log::line( verbose::log, "connection: ", *connection);

               if( message::protocol::compatible< message::domain::disconnect::Request>( connection->protocol()))
               {
                  if( auto correlation = inbound::tcp::send( state, descriptor, message::domain::disconnect::Request{}))
                     state.correlations.emplace_back( correlation, descriptor);
               }
               else
               {
                  handle::connection::lost( state, descriptor);
               }
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
         for( auto descriptor : state.external.descriptors())
            handle::connection::disconnect( state, descriptor);
      }

      void abort( State& state)
      {
         Trace trace{ "gateway::group::inbound::handle::abort"};

         state.runlevel = decltype( state.runlevel())::error;

         // 'kill' all sockets, and try to take care of pending stuff. copy - connection::lost mutates external
         for( auto descriptor : state.external.descriptors())
            handle::connection::lost( state, descriptor);
      }
      

   } // gateway::group::inbound::handle

} // casual