//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "server/context.h"
#include "server/argument.h"

#include "transaction/context.h"

#include "service/call/context.h"
#include "service/conversation/context.h"

#include "common/communication/instance.h"
#include "common/buffer/pool.h"
#include "common/process.h"
#include "common/instance.h"
#include "common/message/service.h"
#include "common/message/conversation.h"
#include "common/execution/context.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

#include "common/log.h"
#include "common/log/category.h"
#include "common/log.h"


#include <algorithm>

namespace casual
{

   namespace server
   {

      namespace local
      {
         namespace
         {
            void advertise( const std::vector< server::Service>& services)
            {
               common::Trace trace{ "server::local::advertise"};

               auto transform_service = []( const server::Service& service)
               {
                  common::message::service::advertise::Service result;
                  result.name = service.name;
                  result.category = service.category;
                  result.transaction = service.transaction;
                  result.visibility = service.visibility;

                  return result;
               };

               common::message::service::Advertise advertise{ common::process::handle()};
               advertise.alias = common::instance::alias();
               advertise.services.add = common::algorithm::transform( services, transform_service);

               common::signal::thread::scope::Mask block{ 
                  common::signal::set::filled( common::code::signal::terminate, common::code::signal::interrupt)};

               common::communication::device::blocking::send( 
                  common::communication::instance::outbound::service::manager::device(), advertise);

               common::log::debug( "advertise: ", advertise);
               common::log::line( common::log::category::event::server, "advertise");
            }

            void finalize( State& state, const common::message::service::call::ACK& ack)
            {
               common::Trace trace{ "server::local::finalize"};

               common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), ack);
               
               common::buffer::pool::holder().clear();
               common::execution::context::clear();
            }

            auto normalize_reply_code( auto& reply) -> decltype( auto)
            {
               using Result = decltype( reply.code.result);
               if( reply.code.result != Result::ok)
                  return reply;

               // if transaction state is _not good_ we need to indicate this on the
               // normal "reply code channel".
               switch( reply.transaction_state)
               {
                  // TODO: not totally sure about the exact correlation between 
                  //    transaction.state -> reply.code.result. For now, we use "the worst".
                  using Enum = decltype( reply.transaction_state);
                  case Enum::ok:
                     break;
                  case Enum::rollback:
                  case Enum::timeout:
                  case Enum::error:
                     reply.code.result = Result::service_error;
                     break;
               }
               return reply;
            };

            auto initialize_transaction( const common::transaction::ID& trid, const server::Service& service)
            {
               common::Trace trace{ "server::local::initialize_transaction"};
               common::log::debug( "trid: ", trid, " - service: ", service);

               // We keep track of callers transaction (can be null-trid).
               transaction::context().caller = trid;

               switch( service.transaction)
               {
                  case service::transaction::Type::automatic:
                  {
                     if( trid)
                        transaction::Context::instance().join( trid);
                     else
                        transaction::Context::instance().start();

                     break;
                  }
                  case service::transaction::Type::branch:
                  {
                     if( trid)
                        transaction::Context::instance().branch( trid);
                     else
                        transaction::Context::instance().start();
                     break;
                  }
                  case service::transaction::Type::join:
                  {
                     transaction::Context::instance().join( trid);
                     break;
                  }
                  case service::transaction::Type::atomic:
                  {
                     transaction::Context::instance().start();
                     break;
                  }
                  default:
                  {
                     common::log::line( common::log::category::error, "unknown transaction semantics for service: ", service);
                     // fallthrough
                  }
                  case service::transaction::Type::none:
                  {
                     // We don't start or join any transactions
                     // (technically we join a null-trid)
                     transaction::Context::instance().join( transaction::ID{ common::process::id()});
                     break;
                  }
               }

               return transaction::context().current().trid;
            }


            auto finalize_transaction( bool commit)
            {
               common::Trace trace{ "server::local::finalize_transaction"};

               // try to wait for all in-flights that are associated with transactions
               casual::service::call::context().finalize( transaction::context().associated());

               return transaction::context().finalize( commit);
            }

            namespace transform
            {
               auto reply( const common::message::service::call::callee::Request& message)
               {
                  auto result = common::message::reverse::type( message);

                  result.buffer = common::buffer::Payload{ nullptr};
                  result.code.result = common::code::xatmi::service_error;

                  return result;
               }

               auto reply( const common::message::conversation::connect::callee::Request& message)
               {
                  common::message::conversation::callee::Send result;

                  result.correlation = message.correlation;
                  result.execution = message.execution;
                  result.buffer = common::buffer::Payload{ nullptr};
                  result.code.result = common::code::xatmi::service_error;

                  return result;
               }


               auto generic_parameter( auto& message, const common::transaction::ID& trid)
               {
                  service::invoke::Parameter result{
                     .service = { .name = message.service.name},
                     .header = std::move( message.header),
                     .parent = message.parent,
                     .payload = std::move( message.buffer)
                  };
                  
                  if( trid)
                     result.flags = service::invoke::Parameter::Flag::in_transaction;

                  return result;

               }

               auto parameter( const common::message::service::call::callee::Request& message, const common::transaction::ID& trid)
               {
                  auto result = transform::generic_parameter( message, trid);

                  using Flag = decltype( message.flags);

                  if( common::flag::contains( message.flags, Flag::no_reply))
                     result.flags |= decltype( result.flags)::no_reply;

                  return result;
               }


               auto parameter( const common::message::conversation::connect::callee::Request& message, const common::transaction::ID& trid)
               {
                  auto result = transform::generic_parameter( message, trid);

                  // set flags
                  {
                     using Flag = service::invoke::Parameter::Flag;
                     result.flags |= Flag::conversation;

                     using Duplex = decltype( message.duplex);
                     casual::assertion( common::algorithm::compare::any( message.duplex, Duplex::send, Duplex::receive), "unexpected duplex: ", message);

                     result.flags |= message.duplex == Duplex::receive ? Flag::receive_only : Flag::send_only;
                  }


                  // reserve descriptor, can "never" fail
                  result.descriptor = casual::service::conversation::context().descriptors().reserve( 
                     message.correlation,
                     message.process,
                     message.duplex,
                     false  // not the initiator
                  );

                  // send reply
                  {
                     auto reply = common::message::reverse::type( message, common::process::handle());
                     common::communication::device::blocking::send( message.process.ipc, reply);
                  }

                  return result;
               }

            } // transform

            void complement_reply( service::invoke::Result&& result, common::message::service::call::Reply& reply)
            {
               common::Trace trace{ "server::local::complement_reply"};
               common::log::debug( "result: ", result);

               reply.code.user = result.code.user;
               reply.buffer = std::move( result.payload);

               if( result.code.result == common::flag::xatmi::Return::success)
               {
                  reply.transaction_state = decltype( reply.transaction_state)::ok;
                  reply.code.result = common::code::xatmi::ok;
               }
               else
               {
                  reply.transaction_state = decltype( reply.transaction_state)::rollback;
                  reply.code.result = common::code::xatmi::service_fail;
               }

               common::log::debug( "reply: ", reply);
            }

            void complement_reply( service::invoke::Result&& result, common::message::conversation::callee::Send& reply)
            {
               common::Trace trace{ "server::local::complement_reply"};
               common::log::debug( "result: ", result);

               reply.code.user = result.code.user;
               reply.buffer = std::move( result.payload);

               // we terminate the conversation -> we're doing a service return.
               reply.duplex = decltype( reply.duplex)::terminated;

               if( result.code.result == common::flag::xatmi::Return::success)
                  reply.code.result = common::code::xatmi::ok;  
               else
                  reply.code.result = common::code::xatmi::service_fail;


               common::log::debug( "reply: ", reply);
            }

            template< typename M>
            void forward( const M& message, service::invoke::Forward&& forward)
            {
               common::Trace trace{ "server::local::forward"};

               if( transaction::context().pending())
                  common::code::raise::error( common::code::xatmi::service_error, "forward with pending transactions - service: ", message.service.name);

               casual::service::Lookup lookup{
                  forward.parameter.service.name,
                  {}, // nill trid
                  decltype( casual::service::lookup::Context::semantic)::no_reply};

               // TODO make this forward work without a copy of payload...
               auto request = message;

               auto target = casual::service::lookup::reply( std::move( lookup));

               request.buffer = std::move( forward.parameter.payload);
               request.service = target.service;

               common::log::debug( "server::local::forward - request:", request);

               common::communication::device::blocking::send( target.process.ipc, request);
            }


            namespace handle
            {
               template< typename Message>
               void generic_call( State& state, Message&& message)
               {
                  common::Trace trace{ "server::local::handle::call"};

                  auto start = platform::time::clock::type::now();

                  common::execution::context::service::set( message.service.name);
                  common::execution::context::span::reset();
                  common::execution::context::parent::service::set( message.parent.service);
                  common::execution::context::parent::span::set( message.parent.span);

                  // set deadline (if any) for further service calls downstream
                  casual::service::call::context().deadline( start, message.deadline.remaining);

                  // Prepare current_trid for later;
                  common::transaction::ID current_trid;

                  // Prepare reply
                  auto reply = local::transform::reply( message);

                  // Make sure we do some cleanup and send ACK to service-manager.
                  auto execute_finalize = common::execute::scope( [&]()
                  {
                     common::message::service::call::ACK ack;

                     ack.correlation = message.correlation;
                     ack.execution = message.execution;
                     ack.metric.span = common::execution::context::get().span;
                     ack.metric.execution = message.execution;
                     ack.metric.service = message.service.logical_name();
                     ack.metric.parent = message.parent;
                     ack.metric.process = common::process::handle();
                     ack.metric.trid = current_trid;

                     ack.metric.start = start;
                     ack.metric.end = platform::time::clock::type::now();

                     // make sure service-manager "gets back" the pending metric
                     ack.metric.pending = message.pending;
                     ack.metric.code = reply.code;

                     local::finalize( state, ack);
                  });

                  auto execute_reply = common::execute::scope( [&]()
                  {
                     if constexpr( std::is_same_v< std::decay_t< Message>, common::message::service::call::callee::Request>)
                     {
                        if( common::flag::contains( message.flags, decltype( message.flags)::no_reply))
                           return;
                     }
                     
                     common::communication::device::blocking::send( message.process.ipc,  local::normalize_reply_code( reply));                        
                  });

                  // If something goes wrong, make sure to rollback before reply with error.
                  // this will execute before execute_reply
                  auto execute_error_reply = common::execute::scope( [&]()
                  {
                     reply.transaction_state = local::finalize_transaction( false);
                  });

                  // Find service
                  auto found = common::algorithm::find( state.services, message.service.name);

                  if( ! found)
                     common::code::raise::error( common::code::xatmi::system, message.service.name, " not present at server - inconsistency between service-manager and server");

                  auto& service = found->second;

                  // initialize transaction, if any
                  current_trid = local::initialize_transaction( message.trid, service);

                  auto parameter = local::transform::parameter( message, current_trid);

                  // transform::parameter( message) may have reserved a descriptor that we need to
                  // unreserve! 
                  auto execute_unreserve_descriptor = common::execute::scope( [descriptor = parameter.descriptor]()
                  {
                     if( descriptor)
                        casual::service::conversation::context().descriptors().unreserve( descriptor);
                  });

                  // call the service
                  try
                  {
                     local::complement_reply( service( std::move( parameter)), reply);
                  }
                  catch( casual::server::service::invoke::Forward& forward)
                  {
                     local::forward( message, std::move( forward));
                     
                     local::finalize_transaction( true);

                     execute_reply.release();
                     execute_error_reply.release();

                     // reply.code.result is used to set outcome in the 'ack'. We might want a _forward_ code?
                     reply.code.result = common::code::xatmi::ok;

                     return;
                  }

                  // Do transaction stuff...
                  // - commit/rollback transaction if service has "auto-transaction"
                  auto execute_transaction = common::execute::scope( [&]()
                  {
                     reply.transaction_state = local::finalize_transaction( reply.transaction_state == decltype( reply.transaction_state)::ok);
                  });

                  // Nothing did go wrong
                  execute_error_reply.release();

                  execute_transaction();
                  execute_reply();
                  execute_unreserve_descriptor();
               }


               auto service_call( State& state)
               {
                  return [ &state]( common::message::service::call::callee::Request&& message)
                  {
                     common::Trace trace{ "server::local::handle::service_call::message"};
                     common::log::debug( "message: ", message);

                     handle::generic_call( state, std::move( message));

                  };
               }

               auto conversation_connect( State& state)
               {
                  return [ &state]( common::message::conversation::connect::callee::Request&& message)
                  {
                     common::Trace trace{ "server::local::handle::conversation_connect::message"};
                     common::log::debug( "message: ", message);

                     handle::generic_call( state, std::move( message));
                  };
               }
               
            } // handle

            
         } // <unnamed>
      } // local

      namespace detail
      {
         // only exposed for unittests
         void finalize_transaction( bool commit)
         {
            local::finalize_transaction( commit);
         }
      } // detail
      

      namespace state
      {
         std::ostream& operator << ( std::ostream& out, const Jump& value)
         {
            return common::stream::write( out, "{ value: ", value.state.value,
               ", code: ", value.state.code,
               ", data: ", value.buffer.data,
               ", size: ", value.buffer.size,
               ", service: ", value.forward.service, '}');
         }
      } // state


      Context& Context::instance()
      {
         static Context singleton;
         return singleton;
      }

      server::dispatch_type Context::initialize( server::Arguments arguments) &
      {
         common::Trace trace{ "server::Context::initialize" };
         common::log::debug( "arguments: ", arguments);

         for( auto& service : arguments.services)
         {
            m_state.physical_services.push_back( service);
            m_state.services.emplace(
               service.name,
               m_state.physical_services.back());
         }

         // configure resources, if any.
         transaction::context().configure( arguments.resources);

         local::advertise( arguments.services);

         return server::dispatch_type{
            local::handle::service_call( m_state),
            local::handle::conversation_connect( m_state),
         };
      }



      Context::Context()
      {
         common::Trace log{ "server::Context instantiated"};
      }


      void Context::jump_return( common::flag::xatmi::Return rval, long rcode, char* data, long len)
      {
         // Prepare buffer.
         // We have to keep state, since there seems not to be any way to send information
         // via longjump...

         m_state.jump.state.value = rval;
         m_state.jump.state.code = rcode;
         m_state.jump.buffer.data = common::buffer::handle::type{ data};
         m_state.jump.buffer.size = len;
         m_state.jump.forward.service.clear();

         common::log::debug( "Context::jump_return - jump state: ", m_state.jump);

         std::longjmp( m_state.jump.environment, state::Jump::Location::c_return);
      }

      void Context::normal_return( common::flag::xatmi::Return rval, long rcode, char* data, long len)
      {
         // Prepare buffer.
         // Essentially the same as jump_return above, but instead of a longjmp
         // this variant returns to the caller. Used by the COBOL api TPRETURN
         // function that is expected to return to its caller, that ultimately
         // returns to the "communications manager" (Casual) without bypassing 
         // the COBOL runtime. 

         m_state.jump.state.value = rval;
         m_state.jump.state.code = rcode;
         m_state.jump.buffer.data = common::buffer::handle::type{ data};
         m_state.jump.buffer.size = len;
         m_state.jump.forward.service.clear();

         m_state.TPRETURN_called = true;

         common::log::debug( "Context::normal_return - jump state: ", m_state.jump);
      }


      void Context::forward( const char* service, char* data, long size)
      {
         m_state.jump.state.value = common::flag::xatmi::Return::success;
         m_state.jump.state.code = 0;
         m_state.jump.buffer.data = common::buffer::handle::type{ data};
         m_state.jump.buffer.size = size;

         m_state.jump.forward.service = service ? service : "";

         common::log::debug( "Context::forward - jump state: ", m_state.jump);

         std::longjmp( m_state.jump.environment, state::Jump::Location::c_forward);
      }

      void Context::advertise( const std::string& service, void (*address)( TPSVCINFO *))
      {
         common::Trace trace{ "server::Context::advertise"};
         common::log::debug( "service: ", service);

         auto prospect = xatmi::service( service, address);

         // validate
         if( prospect.name.size() >= XATMI_SERVICE_NAME_LENGTH)
         {
            prospect.name.resize( XATMI_SERVICE_NAME_LENGTH - 1);
            common::log::line( common::log::category::error, "service name '", service, "' truncated to '", prospect.name, "'");
         }

         if( auto found = common::algorithm::find( m_state.services, prospect.name))
         {
            // service name is already advertised
            // No error if it's the same function
            if( found->second != prospect)
               common::code::raise::error( common::code::xatmi::service_advertised, "service is already advertised - ", prospect.name);
         }
         else
         {
            common::message::service::Advertise message{ common::process::handle()};
            message.alias = common::instance::alias();

            auto is_prospect = [&prospect]( auto& service) { return service == prospect;};

            if( auto found = common::algorithm::find_if( m_state.physical_services, is_prospect))
            {
               m_state.services.emplace( prospect.name, *found);
               message.services.add.push_back( common::message::service::advertise::Service{ 
                  .name = prospect.name, 
                  .category = found->category, 
                  .transaction = found->transaction, 
                  .visibility = found->visibility});
            }
            else
            {
               message.services.add.push_back( { 
                  .name = prospect.name, 
                  .category = prospect.category, 
                  .transaction = prospect.transaction, 
                  .visibility = prospect.visibility});

               m_state.physical_services.push_back( prospect);
               m_state.services.emplace( prospect.name, m_state.physical_services.back());
            }
            common::log::debug( "message: ", message);
            common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), message);
         }
      }

      void Context::unadvertise( const std::string& service)
      {
         common::Trace log{ "server::Context::unadvertise"};

         if( m_state.services.erase( service) != 1)
            common::code::raise::error( common::code::xatmi::no_entry, "service is not currently advertised - ", service);

         common::message::service::Advertise message{ common::process::handle()};
         message.alias = common::instance::alias();
         message.services.remove.emplace_back( service);

         common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), message);
      }


      void Context::configure( const server::Arguments& arguments)
      {
         common::Trace log{ "server::Context::configure"};

         for( auto& service : arguments.services)
         {
            m_state.physical_services.push_back( service);
            m_state.services.emplace(
                  service.name,
                  m_state.physical_services.back());
         }
      }

      namespace local
      {
         namespace
         {
            template< typename S, typename P>
            server::Service* find_physical( S& services, P&& predicate)
            {
               if( auto found = common::algorithm::find_if( services, predicate))
                  return found.data();

               return nullptr;
            }
         } // <unnamed>
      } // local

      server::Service* Context::physical( const std::string& name)
      {
         return local::find_physical(  m_state.physical_services, [&]( const server::Service& s){
            return s.name == name;
         });
      }

      server::Service* Context::physical( const server::xatmi::function_type& function)
      {
         return local::find_physical(  m_state.physical_services, [&]( const server::Service& s){
            return s == xatmi::address( function);
         });

      }

      State& Context::state()
      {
         return m_state;
      }


      void Context::finalize()
      {
         common::buffer::pool::holder().clear();
         common::execution::context::clear();
      }



   } // server
} // casual

