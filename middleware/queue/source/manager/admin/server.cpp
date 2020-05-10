//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/admin/server.h"
#include "queue/manager/admin/services.h"

#include "queue/manager/manager.h"
#include "queue/manager/handle.h"
#include "queue/common/transform.h"

#include "queue/common/log.h"

#include "serviceframework/service/protocol.h"


namespace casual
{
   using namespace common;
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            namespace local
            {
               namespace
               {
                  admin::model::State state( manager::State& state)
                  {
                     admin::model::State result;

                     auto get_queues = [](auto& state)
                     {
                        auto send = [&]( auto& group)
                        {
                           return communication::ipc::blocking::send( group.process.ipc, 
                              common::message::queue::information::queues::Request{ process::handle()});
                        };

                        auto receive = [&]( auto& correlation)
                        {
                           common::message::queue::information::queues::Reply reply;
                           communication::ipc::blocking::receive( ipc::device(), reply, correlation);
                           return reply;
                        };

                        return common::algorithm::transform( algorithm::transform( state.groups, send), receive);
                     };

                     result.queues = transform::queues( get_queues( state));
                     result.groups = transform::groups( state);
                     result.remote = transform::remote( state);

                     return result;
                  }

                  namespace messages
                  {
                     std::vector< model::Message> list( manager::State& state, const std::string& queue)
                     {
                        auto found = common::algorithm::find( state.queues, queue);

                        if( found && ! found->second.empty())
                        {
                           common::message::queue::information::messages::Request request;
                           request.process = common::process::handle();
                           request.qid = found->second.front().queue;

                           return transform::messages( communication::ipc::call( found->second.front().process.ipc, request));
                        }

                        return {};
                     }  

                     std::vector< common::Uuid> remove( manager::State& state, const std::string& queue, std::vector< common::Uuid> ids)
                     {
                        auto found = common::algorithm::find( state.queues, queue);

                        if( found && ! found->second.empty())
                        {
                           common::message::queue::messages::remove::Request request{ common::process::handle()};
                           request.queue = found->second.front().queue;
                           request.ids = std::move( ids);

                           return communication::ipc::call( found->second.front().process.ipc, request).ids;
                        }

                        return {};
                     }
                  } // messages

                  namespace detail
                  {
                     namespace local
                     {
                        struct Group 
                        {
                           struct Queue
                           {
                              std::string name;
                              strong::queue::id id;
                           };

                           process::Handle process;
                           std::vector< Queue> queues;

                           friend bool operator == ( const Group& lhs, const process::Handle& rhs) { return lhs.process == rhs;}
                        };

                        const manager::State::Queue* queue( const manager::State& state, const std::string& name)
                        {
                           auto found = common::algorithm::find( state.queues, name);

                           if( found && ! found->second.empty() && found->second.front().order == 0)
                              return &found->second.front();

                           return nullptr;
                        }

                        std::vector< local::Group> queues( const manager::State& state, std::vector< std::string> names)
                        {
                           std::vector< local::Group> result;

                           auto update_groups = [&]( auto& name)
                           {
                              if( auto queue = local::queue( state, name))
                              {
                                 if( auto found = algorithm::find( result, queue->process))
                                    found->queues.push_back( Group::Queue{ name, queue->queue});
                                 else
                                    result.push_back( Group{ queue->process, { { name, queue->queue}}});
                              }
                           };

                           if( names.empty())
                              names = algorithm::transform( state.queues, []( auto& pair){ return std::get< 0>( pair);});

                           algorithm::for_each( names, update_groups);

                           return result;
                        }
                     } // local
                  } // detail


                  std::vector< model::Affected> restore( manager::State& state, const std::string& name)
                  {
                     Trace trace{ "queue::manager::admin::local::restore"};
                     
                     std::vector< model::Affected> result;

                     if( auto queue = detail::local::queue( state, name))
                     {
                        common::message::queue::restore::Request request;
                        request.process = common::process::handle();
                        request.queues.push_back( queue->queue);

                        auto reply = communication::ipc::call( queue->process.ipc, request);

                        if( ! reply.affected.empty())
                        {
                           auto& restored = reply.affected.front();
                           model::Affected affected;
                           affected.queue.name = name;
                           affected.queue.id = restored.queue;
                           affected.count = restored.count;

                           result.push_back( std::move( affected));
                        }
                     }
                     return result;
                  }

                  std::vector< model::Affected> clear( manager::State& state, std::vector< std::string> queues)
                  {
                     Trace trace{ "queue::manager::admin::local::clear"};

                     struct Queue 
                     {
                        std::string name;
                        const manager::State::Queue* pointer;
                     };

                     auto real_queues = algorithm::transform_if( queues, [&state]( auto& name)
                     {
                        auto queue = detail::local::queue( state, name); 
                        return Queue{ name, queue};
                     },
                     [&state]( auto& name){ return detail::local::queue( state, name) != nullptr;});

                     auto clear_queue = []( auto& queue)
                     {
                        common::message::queue::clear::Request request{ common::process::handle()};
                        request.queues.push_back( queue.pointer->queue);
                        
                        auto reply = communication::ipc::call( queue.pointer->process.ipc, request);

                        model::Affected result;
                        result.queue.name = std::move( queue.name);
                        result.queue.id = queue.pointer->queue;
                        if( ! reply.affected.empty())
                           result.count = reply.affected.front().count;

                        return result;

                     };

                     return algorithm::transform( real_queues, clear_queue);
                  }

                  namespace metric
                  {
                     void reset( manager::State& state, std::vector< std::string> queues)
                     {
                        Trace trace{ "queue::manager::admin::local::metric::reset"};

                        auto groups = detail::local::queues( state, std::move( queues));

                        auto reset_queues = []( auto& group)
                        {
                           common::message::queue::metric::reset::Request request{ common::process::handle()};
                           request.queues = algorithm::transform( group.queues, []( auto& queue){ return queue.id;});
                           communication::ipc::call( group.process.ipc, request);
                        };

                        algorithm::for_each( groups, reset_queues);
                     }
                     
                  } // metric


                  namespace service
                  {
                     auto state( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           return serviceframework::service::user( 
                              serviceframework::service::protocol::deduce( std::move( parameter)), 
                              &local::state, state);
                        };
                     };


                     namespace messages
                     {
                        auto list( manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                              auto queue = protocol.extract< std::string>( "queue");

                              return serviceframework::service::user( 
                                 std::move( protocol), 
                                 &local::messages::list, 
                                 state, 
                                 std::move( queue));
                           };
                        }

                        auto remove( manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                              auto queue = protocol.extract< std::string>( "queue");
                              auto ids = protocol.extract< std::vector< common::Uuid>>( "ids");

                              return serviceframework::service::user( 
                                 std::move( protocol), 
                                 &local::messages::remove, 
                                 state, 
                                 std::move( queue),
                                 std::move( ids));
                           };
                        }
                     } // messages

                     auto restore( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                           auto queue = protocol.extract< std::string>( "queue");

                           return serviceframework::service::user( std::move( protocol), &local::restore, state, queue);
                        };
                     }

                     auto clear( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                           auto queues = protocol.extract< std::vector< std::string>>( "queues");

                           return serviceframework::service::user( std::move( protocol), &local::clear, state, std::move( queues));
                        };
                     }

                     namespace metric
                     {
                        auto reset( manager::State& state)
                        {
                           return [&state]( common::service::invoke::Parameter&& parameter)
                           {
                              auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                              auto queues = protocol.extract< std::vector< std::string>>( "queues");

                              return serviceframework::service::user( std::move( protocol), &local::metric::reset, state, std::move( queues));
                           };
                        }
                     } // metric

                  } // service
               } // <unnamed>
            } // local

            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state,
                        local::service::state( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin
                     },
                     { service::name::messages::list,
                        local::service::messages::list( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin
                     },
                     { service::name::messages::remove,
                        local::service::messages::remove( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin
                     },
                     { service::name::restore,
                        local::service::restore( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin
                     },
                     { service::name::clear,
                        local::service::clear( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin
                     },
                     { service::name::metric::reset,
                        local::service::metric::reset( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin
                     },

               }};
            }
         } // admin
      } // manager
   } // queue


} // casual
