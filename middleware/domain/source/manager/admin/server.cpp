//!
//! casual
//!


#include "domain/manager/admin/server.h"
#include "domain/manager/admin/vo.h"
#include "domain/manager/handle.h"
#include "domain/manager/persistent.h"
#include "domain/transform.h"


#include "sf/service/protocol.h"

#include "xatmi.h"

namespace casual
{
   using namespace common;


   namespace domain
   {

      namespace manager
      {
         namespace admin
         {
            namespace local
            {
               namespace
               {
                  namespace scale
                  {
                     std::vector< vo::scale::Instances> instances( manager::State& state, std::vector< vo::scale::Instances> instances)
                     {
                        std::vector< vo::scale::Instances> result;

                        message::domain::scale::Executable message;

                        auto extract = []( auto& instance, auto& entites, auto& output){

                           auto found = range::find_if( entites, [&instance]( auto& e){
                              return e.alias == instance.alias;
                           });

                           if( found)
                           {
                              message::domain::scale::Executable::Scale scale;
                              scale.id = common::id::underlaying( found->id);
                              scale.instances = instance.instances;
                              output.push_back( std::move( scale));
                              return true;
                           }
                           return false;
                        };

                        for( auto& instance : instances)
                        {
                           if( extract( instance, state.servers, message.servers) ||
                              extract( instance, state.executables, message.executables))
                           {
                              result.push_back( std::move( instance));
                           }
                        }

                        //
                        // Push the work-task to our queue, and It'll be processed as soon
                        // as possible
                        //
                        communication::ipc::inbound::device().push( std::move( message));

                        return result;
                     }

                  } // scale

                  namespace service
                  {
                     common::service::invoke::Result state( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = sf::service::protocol::deduce( std::move( parameter));

                        manager::admin::vo::State (*function)(const manager::State&) = casual::domain::transform::state;

                        auto result = sf::service::user( protocol, function, state);

                        protocol << CASUAL_MAKE_NVP( result);

                        return protocol.finalize();
                     }


                     common::service::invoke::Result scale( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = sf::service::protocol::deduce( std::move( parameter));

                        std::vector< vo::scale::Instances> instances;
                        protocol >> CASUAL_MAKE_NVP( instances);

                        auto result = sf::service::user( protocol, &scale::instances, state, instances);

                        protocol << CASUAL_MAKE_NVP( result);
                        return protocol.finalize();
                     }


                     common::service::invoke::Result shutdown( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = sf::service::protocol::deduce( std::move( parameter));

                        sf::service::user( protocol, &handle::shutdown, state);

                        return protocol.finalize();
                     }

                     common::service::invoke::Result persist( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = sf::service::protocol::deduce( std::move( parameter));

                        sf::service::user( protocol,
                              static_cast< void(*)( const manager::State&)>( persistent::state::save),
                              state);

                        return protocol.finalize();
                     }

                  } // service
               } // <unnamed>
            } // local

            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state(),
                        std::bind( &local::service::state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     },
                     { service::name::scale::instances(),
                           std::bind( &local::service::scale, std::placeholders::_1, std::ref( state)),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::shutdown(),
                           std::bind( &local::service::shutdown, std::placeholders::_1, std::ref( state)),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
                     { service::name::configuration::persist(),
                           std::bind( &local::service::persist, std::placeholders::_1, std::ref( state)),
                           common::service::transaction::Type::none,
                           common::service::category::admin()
                     },
               }};
            }
         } // admin
      } // manager
   } // gateway
} // casual
