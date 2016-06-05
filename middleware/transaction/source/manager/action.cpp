//!
//! casual
//!

#include "transaction/manager/action.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/admin/transform.h"

#include "common/process.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/internal/trace.h"
#include "common/server/handle.h"


#include "sf/log.h"

#include <string>


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace action
      {
         void configure( State& state, const std::string& resource_file)
         {

            {

               common::Trace trace( "configure", log::internal::transaction);

               message::domain::configuration::transaction::resource::Request request;
               request.scope = message::domain::configuration::transaction::resource::Request::Scope::all;
               request.process = process::handle();


               auto configuration = communication::ipc::call( communication::ipc::domain::manager::device(), request);


               //
               // configure state
               //
               state::configure( state, configuration, resource_file);
            }

            {
               Trace trace( "event registration", log::internal::transaction);

               message::domain::process::termination::Registration message;
               message.process = common::process::handle();

               ipc::device().blocking_send( communication::ipc::domain::manager::device(), message);
            }

         }



         namespace resource
         {

            void Instances::operator () ( state::resource::Proxy& proxy)
            {
               trace::internal::Scope trace( "resource::Instances::operator()", common::log::internal::transaction);

               common::log::internal::transaction << "update instances for resource: " << proxy << std::endl;

               auto count = static_cast< long>( proxy.concurency - proxy.instances.size());

               if( count > 0)
               {
                  while( count-- > 0)
                  {
                     auto& info = m_state.xaConfig.at( proxy.key);

                     state::resource::Proxy::Instance instance;//( proxy.id);
                     instance.id = proxy.id;

                     instance.process.pid = process::spawn(
                           info.server,
                           {
                                 "--tm-queue", std::to_string( ipc::device().id()),
                                 "--rm-key", info.key,
                                 "--rm-openinfo", proxy.openinfo,
                                 "--rm-closeinfo", proxy.closeinfo,
                                 "--rm-id", std::to_string( proxy.id),
                           }
                        );

                     instance.state( state::resource::Proxy::Instance::State::started);

                     proxy.instances.push_back( std::move( instance));
                  }
               }
               else
               {
                  auto end = std::end( proxy.instances);

                  for( auto& instance : range::make( end + count, end))
                  {
                     switch( instance.state())
                     {
                        case state::resource::Proxy::Instance::State::absent:
                        case state::resource::Proxy::Instance::State::started:
                        {

                           log::internal::transaction << "Instance has not register yet. We, kill it...: " << instance << std::endl;

                           process::lifetime::terminate( { instance.process.pid});
                           instance.state( state::resource::Proxy::Instance::State::shutdown);
                           break;
                        }
                        case state::resource::Proxy::Instance::State::shutdown:
                        {
                           log::internal::transaction << "instance already in shutdown state - " << instance << std::endl;
                           break;
                        }
                        default:
                        {
                           log::internal::transaction << "shutdown instance: " << instance << std::endl;


                           instance.state( state::resource::Proxy::Instance::State::shutdown);

                           if( ! ipc::device().non_blocking_send( instance.process.queue, message::shutdown::Request{}))
                           {
                              //
                              // We couldn't send shutdown for some reason, we put the message in 'persistent-replies' and
                              // hope to send it later...
                              //
                              log::warning << "failed to send shutdown to instance: " << instance << " - action: try send it later" << std::endl;

                              m_state.persistentReplies.emplace_back( instance.process.queue, message::shutdown::Request{});
                           }
                           break;
                        }
                     }
                  }
               }
            }

            std::vector< vo::resource::Proxy> insances( State& state, std::vector< vo::update::Instances> instances)
            {
               std::vector< vo::resource::Proxy> result;

               //
               // Make sure we only update a specific RM one time
               //
               for( auto& directive : range::unique( range::sort( instances)))
               {
                  try
                  {

                     auto& resource = state.get_resource( directive.id);
                     resource.concurency = directive.instances;

                     Instances{ state}( resource);

                     result.push_back( transform::resource::Proxy{}( resource));

                  }
                  catch( const common::exception::invalid::Argument&)
                  {
                     //
                     // User did not use correct resource-id. We propagate this by not including
                     // the resource in the result
                     //
                  }

               }

               return result;
            }

            namespace instance
            {
               bool request( State& state, const common::communication::message::Complete& message, state::resource::Proxy::Instance& instance)
               {
                  Trace trace{ "transaction::action::resource::instance::request", log::internal::transaction};

                  if( ipc::device().non_blocking_push( instance.process.queue, message))
                  {
                     instance.state( state::resource::Proxy::Instance::State::busy);
                     instance.statistics.roundtrip.start( common::platform::clock_type::now());
                     return true;
                  }
                  return false;

               }
            } // instance

            bool request( State& state, state::pending::Request& message)
            {
               Trace trace{ "transaction::action::resource::request", log::internal::transaction};

               decltype( message.resources) resources;
               std::swap( message.resources, resources);

               for( auto&& id : resources)
               {
                  auto found = state.idle_instance( id);

                  if( found )
                  {
                     if( ! resource::instance::request( state, message.message, *found))
                     {
                        common::log::internal::transaction << "failed to send resource request - type: " << message.message.type << " to: " << found->process << "\n";
                        message.resources.push_back( id);
                     }
                  }
                  else
                  {
                     message.resources.push_back( id);
                  }
               }

               return message.resources.empty();
            }

         } // resource


         namespace persistent
         {

            bool Send::operator () ( state::pending::Reply& message) const
            {
               try
               {
                  if( ! ipc::device().non_blocking_push( message.target, message.message))
                  {
                     common::log::internal::transaction << "failed to send reply - type: " << message.message.type << " to: " << message.target << "\n";
                     return false;
                  }
               }
               catch( const exception::queue::Unavailable&)
               {
                  common::log::error << "failed to send reply - target: " << message.target << ", message: " << message.message << " - TODO: rollback transaction?\n";
                  //
                  // ipc-queue has been removed...
                  // TODO: deduce from message.message.type what we should do
                  //  We should rollback if we are in a prepare stage?
                  //
               }
               return true;
            }

            bool Send::operator () ( state::pending::Request& message) const
            {
               return resource::request( m_state, message);
            }

         } // pending

      } // action
   } //transaction
} // casual
