//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "common/server_context.h"

#include "common/queue.h"
#include "common/buffer/pool.h"
#include "common/calling_context.h"
#include "common/process.h"

#include "common/log.h"
#include "common/error.h"



#include "xatmi.h"

#include <algorithm>



namespace casual
{
   namespace common
   {
      namespace server
      {
         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }



         Context::Context()
         {
            trace::internal::Scope log{ "server::Context instansiated"};
         }


         void Context::longJumpReturn( int rval, long rcode, char* data, long len, long flags)
         {
            log::internal::debug << "tpreturn - rval: " << rval << " - rcode: " << rcode << " - data: @" << static_cast< void*>( data) << " - len: " << len << " - flags: " << flags << std::endl;

            //
            // Prepare buffer.
            // We have to keep state, since there seems not to be any way to send information
            // via longjump...
            //

            m_state.jump.state.value = rval;
            m_state.jump.state.code = rcode;
            m_state.jump.state.data = data;
            m_state.jump.state.len = len;

            longjmp( m_state.long_jump_buffer, 1);
         }

         void Context::advertiseService( const std::string& name, void (*adress)( TPSVCINFO *))
         {
            trace::internal::Scope trace{ "server::Context advertise service " + name};

            Service prospect{ name, adress};


            //
            // validate
            //


            if( prospect.name.size() >= XATMI_SERVICE_NAME_LENGTH)
            {
               prospect.name.resize( XATMI_SERVICE_NAME_LENGTH - 1);
               log::warning << "service name '" << name << "' truncated to '" << prospect.name << "'";
            }

            auto found = range::find( m_state.services, prospect.name);

            if( found)
            {
               //
               // service name is already advertised
               // No error if it's the same function
               //
               if( found->second != prospect)
               {
                  throw common::exception::xatmi::service::AllreadyAdvertised( "service name: " + prospect.name);
               }

            }
            else
            {
               // TODO: find the function and get information from it (transaction semantics and such)

               message::service::Advertise message;

               message.process = process::handle();
               message.serverPath = process::path();
               message.services.emplace_back( prospect.name);

               // TODO: make it consistence safe...
               queue::blocking::Writer writer( ipc::broker::id());
               writer( message);

               m_state.services.emplace( prospect.name, std::move( prospect));
            }
         }

         void Context::unadvertiseService( const std::string& name)
         {
            trace::internal::Scope log{ "server::Context unadvertise service" + name};

            if( m_state.services.erase( name) != 1)
            {
               throw common::exception::xatmi::service::NoEntry( "service name: " + name);
            }

            message::service::Unadvertise message;
            message.process = process::handle();
            message.services.push_back( message::Service( name));

            queue::blocking::Writer writer( ipc::broker::id());
            writer( message);
         }


         State& Context::state()
         {
            return m_state;
         }


         void Context::finalize()
         {
            buffer::pool::Holder::instance().clear();
            calling::Context::instance().currentService( "");
         }



      } // server

      namespace callee
      {
         namespace handle
         {
            namespace policy
            {
               void Default::connect( message::server::connect::Request& message, const std::vector< transaction::Resource>& resources)
               {

                  //
                  // Let the broker know about us, and our services...
                  //
                  message.process = process::handle();
                  message.path = common::process::path();
                  blocking_broker_writer brokerWriter;
                  brokerWriter( message);
                  //
                  // Wait for configuration reply
                  //
                  queue::blocking::Reader reader( ipc::receive::queue());
                  message::server::connect::Reply reply;
                  reader( reply);

                  transaction::Context::instance().set( resources);

               }

               void Default::reply( platform::queue_id_type id, message::service::Reply& message)
               {
                  reply_writer writer{ id };
                  writer( message);
               }

               void Default::ack( const message::service::callee::Call& message)
               {
                  message::service::ACK ack;
                  ack.process = process::handle();
                  ack.service = message.service.name;
                  blocking_broker_writer brokerWriter;
                  brokerWriter( ack);
               }


               void Default::disconnect()
               {
                  // TODO: we shall get rid of disconnect messages, and rely on terminate-signal

                  message::server::Disconnect message;
                  message.process = process::handle();
                  //
                  // we can't block here...
                  //
                  non_blocking_broker_writer brokerWriter;
                  brokerWriter( message);
               }

               void Default::statistics( platform::queue_id_type id, message::monitor::Notify& message)
               {
                  monitor_writer writer{ id};
                  writer( message);
               }

               void Default::transaction( const message::service::callee::Call& message, const server::Service& service)
               {
                  log::internal::debug << "service: " << service << std::endl;

                  switch( service.transaction)
                  {
                     case server::Service::cAuto:
                     {
                        transaction::Context::instance().joinOrStart( message.trid);

                        break;
                     }
                     case server::Service::cJoin:
                     {
                        if( message.trid)
                        {
                           transaction::Context::instance().joinOrStart( message.trid);
                        }

                        break;
                     }
                     case server::Service::cAtomic:
                     {
                        transaction::Context::instance().joinOrStart( common::transaction::ID::create());
                        break;
                     }
                     default:
                     {
                        //
                        // We don't start or join any transactions
                        //
                        break;
                     }

                  }

               }

               void Default::transaction( message::service::Reply& message)
               {
                  transaction::Context::instance().finalize( message);
               }

            } // policy
         } // handle
      } // callee
   } // common
} // casual

