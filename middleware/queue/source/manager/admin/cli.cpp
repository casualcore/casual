//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/admin/cli.h"

#include "queue/manager/admin/model.h"
#include "queue/manager/admin/services.h"

#include "queue/api/queue.h"
#include "queue/common/queue.h"

#include "common/transaction/id.h"

#include "common/argument.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"
#include "common/execute.h"
#include "common/exception/capture.h"
#include "common/serialize/create.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/message/dispatch/handle.h"

#include "casual/cli/pipe.h"
#include "casual/cli/message.h"
#include "casual/cli/state.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace queue
   {

      namespace local
      {
         namespace
         {
            namespace global
            {
               struct
               {
                  transaction::ID trid;
               } state;

            } // global

            struct State
            {
               bool force = false;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( force);
               )
            };

            namespace normalize
            {
               std::string timestamp( const platform::time::point::type& time)
               {
                  if( time != platform::time::point::limit::zero())
                     return chronology::utc::offset( time);

                  return "-";;
               }

               namespace instance
               {
                  enum struct State : std::uint16_t
                  {
                     internal,
                     external,
                  };
                  constexpr std::string_view description( State value) noexcept
                  {
                     switch( value)
                     {
                        case State::internal: return "internal";
                        case State::external: return "external";
                     }
                     return "<unknown>";
                  }
               } // instance

               struct Instance : compare::Order< Instance>
               {
                  instance::State state = instance::State::internal;
                  std::string queue;
                  process::Handle process;
                  std::string alias;
                  std::string description;
                  platform::size::type order{};

                  auto tie() const noexcept { return std::tie( queue, state, alias, description);}

               };

               auto instances( const manager::admin::model::State& state)
               {
                  auto result = algorithm::transform( state.queues, [ &state]( auto& queue)
                  {
                     if( auto found = algorithm::find( state.groups, queue.group))
                     {
                        Instance result; 
                        result.state = instance::State::internal;
                        result.queue = queue.name;
                        result.process = found->process;
                        result.alias = found->alias;
                        return result;
                     }
                     common::code::raise::error( common::code::casual::internal_unexpected_value, "cli model - failed to find group for: ", queue);
                  });

                  algorithm::transform( state.remote.queues, result, [ &state]( auto& queue)
                  {
                     if( auto found = algorithm::find( state.remote.domains, queue.process.ipc))
                     {
                        Instance result; 
                        result.state = instance::State::external;
                        result.queue = queue.name;
                        result.process = found->process;
                        result.alias = found->alias;
                        result.order = found->order;
                        result.description = found->description;
                        return result;
                     }
                     common::code::raise::error( common::code::casual::internal_unexpected_value, "cli model - failed to find group for: ", queue);
                  });

                  algorithm::sort( result);

                  return result;
               }

            } // normalize

            namespace call
            {
               manager::admin::model::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  return call( manager::admin::service::name::state).extract< manager::admin::model::State>();
               }

               std::vector< manager::admin::model::Message> messages( std::string_view queue)
               {
                  serviceframework::service::protocol::binary::Call call;
                  return call( manager::admin::service::name::messages::list, queue).extract< std::vector< manager::admin::model::Message>>();
               }

               std::vector< common::transaction::global::ID> recover( const std::vector< common::transaction::global::ID>& gtrids,
                  ipc::message::group::message::recovery::Directive directive)
               {
                  using Call = serviceframework::service::protocol::binary::Call;
                  return Call{}( manager::admin::service::name::recover,
                     std::move( gtrids),
                     std::move( directive)).extract< std::vector< common::transaction::global::ID>>();
               }
            } // call


            namespace format
            {
               auto empty_representation()
               {
                  if( ! terminal::output::directive().porcelain())
                     return "-";

                  return "";
               }
               
               // this kind of stuff should maybe be in the terminal abstraction?
               auto hyphen_if_empty( std::string_view value) -> std::string_view 
               {
                  if( value.empty())
                     return empty_representation();
                  return value;
               }

               auto messages()
               {
                  auto format_state = []( auto& message)
                  {
                     using Enum = decltype( message.state);
                     switch( message.state)
                     {
                        case Enum::enqueued: return 'E';
                        case Enum::committed: return 'C';
                        case Enum::dequeued: return 'D';
                     }
                     return '?';
                  };

                  auto format_trid = []( auto& message) { return transcode::hex::encode( message.trid);};
                  auto format_type = []( auto& message) { return message.type;};
                  auto format_timestamp = []( auto& message) { return normalize::timestamp( message.timestamp);};
                  auto format_available = []( auto& message) { return normalize::timestamp( message.available);};

                  return terminal::format::formatter< manager::admin::model::Message>::construct(
                     terminal::format::column( "id", []( auto& message) { return message.id;}, terminal::color::yellow),
                     terminal::format::column( "S", format_state, terminal::color::no_color),
                     terminal::format::column( "size", []( auto& message) { return message.size;}, terminal::color::cyan, terminal::format::Align::right),
                     terminal::format::column( "trid", format_trid, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rd", []( auto& message) { return message.redelivered;}, terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "type", format_type, terminal::color::no_color),
                     terminal::format::column( "reply", []( auto& message) { return message.reply;}, terminal::color::no_color),
                     terminal::format::column( "available", format_available, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "timestamp", format_timestamp, terminal::color::blue, terminal::format::Align::right)
                  );
               }

               auto groups()
               {
                  auto format_alias = []( auto& group) { return hyphen_if_empty( group.alias);};
                  auto format_pid = []( auto& group) { return group.process.pid;};
                  auto format_ipc = []( auto& group) { return group.process.ipc;};
                  auto format_queuebase = []( auto& group) { return hyphen_if_empty( group.queuebase);};
                  auto format_size = []( auto& group) { return group.size.current;};
                  auto format_capacity = []( auto& group) -> std::string
                  {
                     if( group.size.capacity)
                        return common::string::compose( group.size.capacity.value());
                     return "-";
                  };

                  return terminal::format::formatter< manager::admin::model::Group>::construct(
                     terminal::format::column( "alias", format_alias, terminal::color::yellow),
                     terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "ipc", format_ipc, terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "queuebase", format_queuebase, terminal::color::cyan),
                     terminal::format::column( "size", format_size, terminal::format::Align::right),
                     terminal::format::column( "capacity", format_capacity, terminal::format::Align::right)
                  );
               }

               auto affected()
               {
                  auto format_name = []( auto& value) { return value.queue;};

                  return terminal::format::formatter< queue::restore::Affected>::construct(
                     terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::right),
                     terminal::format::column( "count", std::mem_fn( &queue::restore::Affected::count), terminal::color::green)
                  );
               }


               auto queues( const manager::admin::model::State& state)
               {
                  using second_t = std::chrono::duration< double>;

                  auto format_retry_delay = []( auto& queue)
                  {
                     return std::chrono::duration_cast< second_t>( queue.retry.delay).count();
                  };
                  
                  auto format_group = [&]( auto& queue)
                  {
                     return algorithm::find( state.groups, queue.group).at( 0).alias;
                  };

                  auto avg_size = []( auto& queue)
                  {
                     return queue.count == 0 ? 0 : queue.size / queue.count;
                  };

                  auto enable = []( auto& queue) -> std::string_view
                  {
                     if( queue.enable.enqueue && queue.enable.dequeue)
                        return "ED";
                     if( queue.enable.enqueue)
                        return "E";
                     if( queue.enable.dequeue)
                        return "D";
                     if( terminal::output::directive().porcelain())
                        return {};
                     return "-";
                  };
                  
                  if( ! terminal::output::directive().porcelain())
                  {
                     return terminal::format::formatter< manager::admin::model::Queue>::construct(
                        terminal::format::column( "name", []( const auto& q){ return q.name;}, terminal::color::yellow),
                        terminal::format::column( "group", format_group),
                        terminal::format::column( "rc", []( const auto& q){ return q.retry.count;}, common::terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "rd", format_retry_delay, common::terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "count", []( const auto& q){ return q.count;}, terminal::color::white, terminal::format::Align::right),
                        terminal::format::column( "size", []( const auto& q){ return q.size;}, common::terminal::color::white, terminal::format::Align::right),
                        terminal::format::column( "avg", avg_size, common::terminal::color::white, terminal::format::Align::right),
                        terminal::format::column( "E", enable, common::terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "EQ", []( auto& q){ return q.metric.enqueued;}, common::terminal::color::cyan, terminal::format::Align::right),
                        terminal::format::column( "DQ", []( auto& q){ return q.metric.dequeued;}, common::terminal::color::cyan, terminal::format::Align::right),
                        terminal::format::column( "UC", []( const auto& q){ return q.uncommitted;}, common::terminal::color::magenta, terminal::format::Align::right),
                        terminal::format::column( "last", []( auto& q){ return normalize::timestamp( q.last);}, common::terminal::color::blue)
                     );
                  }
                  else
                  {
                     return terminal::format::formatter< manager::admin::model::Queue>::construct(
                        terminal::format::column( "name", []( const auto& q){ return q.name;}),
                        terminal::format::column( "group", format_group),
                        terminal::format::column( "rc", []( const auto& q){ return q.retry.count;}),
                        terminal::format::column( "rd", format_retry_delay),
                        terminal::format::column( "count", []( const auto& q){ return q.count;}),
                        terminal::format::column( "size", []( const auto& q){ return q.size;}),
                        terminal::format::column( "avg", avg_size),
                        terminal::format::column( "EQ", []( auto& q){ return q.metric.enqueued;}),
                        terminal::format::column( "DQ", []( auto& q){ return q.metric.dequeued;}),
                        terminal::format::column( "UC", []( const auto& q){ return q.uncommitted;}),
                        terminal::format::column( "last", []( auto& q){ return normalize::timestamp( q.last);}),
                        terminal::format::column( "E", enable)
                     );
                  }

               }

               namespace remote
               {
                  //! @deprecated
                  auto queues( const manager::admin::model::State& state)
                  {
                     auto format_pid = [&]( auto& queue){ return queue.process.pid;};

                     auto format_name = []( auto& queue){ return queue.name;};
         
         
                     return terminal::format::formatter< manager::admin::model::remote::Queue>::construct(
                        terminal::format::column( "name", format_name, terminal::color::yellow),
                        terminal::format::column( "pid", format_pid, common::terminal::color::blue)
                     );
                  }
                  
               } // remote

               namespace queue
               {
                  auto instances()
                  {
                     struct format_state
                     {
                        std::size_t width( const normalize::Instance& instance, const std::ostream&) const
                        {
                           return description( instance.state).size();
                        }

                        void print( std::ostream& out, const normalize::Instance& instance, std::size_t width) const
                        {
                           out << std::setfill( ' ');

                           using State = decltype( instance.state);
                           switch( instance.state)
                           {
                              case State::internal: out << std::left << std::setw( width) << terminal::color::green << "internal"; break;
                              case State::external: out << std::left << std::setw( width) << terminal::color::cyan << "external"; break;
                           }
                        }
                     };

                     auto format_queue = []( auto& instance){ return instance.queue;};

                     auto format_pid = []( auto& instance){ return instance.process.pid;};

                     auto format_alias = []( auto& instance){ return instance.alias;};
                     
                     auto format_description = []( auto& instance){ return hyphen_if_empty( instance.description);};
         
                     return terminal::format::formatter< local::normalize::Instance>::construct(
                        terminal::format::column( "queue", format_queue, terminal::color::yellow),
                        terminal::format::custom::column( "state", format_state{}),
                        terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                        terminal::format::column( "alias", format_alias, terminal::color::blue, terminal::format::Align::left),
                        terminal::format::column( "description", format_description, terminal::color::yellow, terminal::format::Align::left)
                     );
                  }
                  
               } // queue

               namespace forward
               {
                  auto column_alias = []()
                  {
                     return terminal::format::column( "alias", []( auto& forward){ return forward.alias;}, terminal::color::yellow);
                  };

                  auto column_group = []( auto& groups)
                  {
                     return terminal::format::column( "group", [&groups]( auto& forward)
                     { 
                        if( auto found = algorithm::find( groups, forward.group))
                           return found->alias;
                        return std::to_string( forward.group.value());
                     }, terminal::color::white);
                  };

                  auto column_enabled = []()
                  {
                     // todo: is this the best title?
                     return terminal::format::column( "S", []( auto& forward)
                     {
                        return forward.enabled ? "E" : "D";
                     });
                  };
                  
                  auto column_configured_instances = []()
                  { 
                     return terminal::format::column( "CI", []( auto& forward)
                     { 
                        return forward.instances.configured;
                     }, terminal::color::white, terminal::format::Align::right);
                  };

                  auto column_running_instances = []()
                  { 
                     return terminal::format::column( "I", []( auto& forward)
                     { 
                        return forward.instances.running;
                     }, terminal::color::white, terminal::format::Align::right);
                  };

                  auto column_source = []()
                  {
                     return terminal::format::column( "source", []( auto& forward){ return forward.source;}, terminal::color::white);
                  };

                  auto column_commit = []()
                  {
                     return terminal::format::column( "commits", []( auto& forward)
                     { 
                        return forward.metric.commit.count;
                     }, terminal::color::cyan, terminal::format::Align::right);
                  };

                  auto column_rollback = []()
                  {
                     return terminal::format::column( "rollbacks", []( auto& forward)
                     { 
                        return forward.metric.rollback.count;
                     }, terminal::color::cyan, terminal::format::Align::right);
                  };

                  auto column_last = []()
                  {
                     return terminal::format::column( "last", []( auto& forward)
                     {
                        return local::normalize::timestamp( std::max( forward.metric.commit.last, forward.metric.rollback.last));
                     }, terminal::color::blue);
                  };

                  using time_type = std::chrono::duration< double>;

                  auto services( const manager::admin::model::State& state)
                  {
                     auto column_target = []()
                     {
                        return terminal::format::column( "target", []( auto& forward){ return forward.target.service;}, terminal::color::white);
                     };

                     auto column_reply_name = []()
                     {
                        return terminal::format::column( "reply", []( auto& forward)
                        { 
                           if( forward.reply)
                              return forward.reply.value().queue;
                           
                           return std::string{ "-"};
                        }, terminal::color::cyan);
                     };

                     auto column_reply_delay = []()
                     {
                        return terminal::format::column( "delay", []( auto& forward)
                        { 
                           if( ! forward.reply)
                              return std::string{ "-"};

                           return std::to_string( std::chrono::duration_cast< time_type>( forward.reply.value().delay).count());
                        }, terminal::color::cyan, terminal::format::Align::right);
                     };

                     if( ! terminal::output::directive().porcelain())
                     {
                        return terminal::format::formatter< manager::admin::model::forward::Service>::construct(
                           column_alias(),
                           column_group( state.forward.groups),
                           column_source(),
                           column_target(),
                           column_reply_name(),
                           column_reply_delay(),
                           column_enabled(),
                           column_configured_instances(),
                           column_running_instances(),
                           column_commit(),
                           column_rollback(),
                           column_last()
                        );
                     }
                     else
                     {
                        return terminal::format::formatter< manager::admin::model::forward::Service>::construct(
                           column_alias(),
                           column_group( state.forward.groups),
                           column_source(),
                           column_target(),
                           column_reply_name(),
                           column_reply_delay(),
                           column_configured_instances(),
                           column_running_instances(),
                           column_commit(),
                           column_rollback(),
                           column_last(),
                           column_enabled()
                        );
                     }
                  }
   
                  auto queues( const manager::admin::model::State& state)
                  {
                     auto column_target = []()
                     {
                        return terminal::format::column( "target", []( auto& forward){ return forward.target.queue;}, terminal::color::white);
                     };

                     auto column_target_delay = []()
                     {
                        return terminal::format::column( "delay", []( auto& forward)
                        { 
                           return std::chrono::duration_cast< time_type>( forward.target.delay).count();
                        }, terminal::color::white, terminal::format::Align::right);
                     };

                     if( ! terminal::output::directive().porcelain())
                     {
                        return terminal::format::formatter< manager::admin::model::forward::Queue>::construct(
                           column_alias(),
                           column_group( state.forward.groups),
                           column_source(),
                           column_target(),
                           column_target_delay(),
                           column_enabled(),
                           column_configured_instances(),
                           column_running_instances(),
                           column_commit(),
                           column_rollback(),
                           column_last()
                        );
                     }
                     else
                     {
                        return terminal::format::formatter< manager::admin::model::forward::Queue>::construct(
                           column_alias(),
                           column_group( state.forward.groups),
                           column_source(),
                           column_target(),
                           column_target_delay(),
                           column_configured_instances(),
                           column_running_instances(),
                           column_commit(),
                           column_rollback(),
                           column_last(),
                           column_enabled()
                        );
                     }
                  }

                  auto groups( const manager::admin::model::State& state)
                  {
                     auto column_pid = []()
                     {
                        return terminal::format::column( "pid", []( auto& group){ return group.process.pid;}, terminal::color::white);
                     };

                     auto column_services = [&state]()
                     {
                        return terminal::format::column( "services", [&state]( auto& group)
                        { 
                           return algorithm::count( state.forward.services, group.process.pid);

                        }, terminal::color::blue, terminal::format::Align::right);
                     };

                     auto column_queues = [&state]()
                     {
                        return terminal::format::column( "queues", [&state]( auto& group)
                        { 
                           return algorithm::count( state.forward.queues, group.process.pid);

                        }, terminal::color::blue, terminal::format::Align::right);
                     };
                     
                     auto aggregate = [&state]( auto pid, auto extractor)
                     {
                        auto accumulate = [extractor, pid]( auto& forwards)
                        {
                           return algorithm::accumulate( forwards, decltype( extractor( range::front( forwards))){}, [pid, extractor]( auto result, auto& forward)
                           {
                              if( forward == pid)
                                 return result + extractor( forward);
                              return result;
                           });
                        };

                        return accumulate( state.forward.queues) + accumulate( state.forward.services);
                     };

                     auto column_commit = [=]()
                     {
                        return terminal::format::column( "commits", [=]( auto& group)
                        { 
                           return aggregate( group.process.pid, []( auto& forward)
                           { 
                              return forward.metric.commit.count;
                           });
                        }, terminal::color::cyan, terminal::format::Align::right);
                     };

                     auto column_rollback = [=]()
                     {
                        return terminal::format::column( "rollbacks", [=]( auto& group)
                        { 
                           return aggregate( group.process.pid, []( auto& forward)
                           { 
                              return forward.metric.rollback.count;
                           });
                        }, terminal::color::cyan, terminal::format::Align::right);
                     };


                     auto column_last = [&state]()
                     {
                        return terminal::format::column( "last", [&state]( auto& group)
                        { 
                           platform::time::point::type result{};
                           auto last = [&result, pid = group.process.pid]( auto& forward)
                           {
                              auto max = std::max( forward.metric.commit.last, forward.metric.rollback.last);

                              if( forward == pid && result < max)
                                 result = max;
                           };

                           algorithm::for_each( state.forward.queues, last);
                           algorithm::for_each( state.forward.services, last);

                           return local::normalize::timestamp( result);
                           
                        }, terminal::color::blue);
                     };

                     if( ! terminal::output::directive().porcelain())
                     {
                        return terminal::format::formatter< manager::admin::model::forward::Group>::construct(
                           column_alias(),
                           column_pid(),
                           column_enabled(),
                           column_services(),
                           column_queues(),
                           column_commit(),
                           column_rollback(),
                           column_last()
                        );
                     }
                     else
                     {
                        return terminal::format::formatter< manager::admin::model::forward::Group>::construct(
                           column_alias(),
                           column_pid(),
                           column_services(),
                           column_queues(),
                           column_commit(),
                           column_rollback(),
                           column_last(),
                           column_enabled()
                        );
                     }
                  }

               } // forward

            } // format

            auto queues() 
            {
               auto state = call::state();

               return algorithm::transform( state.queues, []( auto& q)
               {
                  return std::move( q.name);
               });
            }

            namespace complete
            {
               auto queues = []( auto& values, bool help) -> std::vector< std::string>
               { 
                  if( help) 
                     return { "<queue>"};
                     
                  return local::queues();
               };
            } // complete

            namespace transform
            {
               template< typename M>
               auto message( M&& message)
               {
                  auto transform_attributes = []( auto&& value)
                  {
                     cli::message::queue::message::Attributes result;
                     result.properties = std::move( value.properties);
                     result.reply = std::move( value.reply);
                     result.available = value.available;
                     return result;
                  };

                  cli::message::queue::Message result;
                  result.id = message.id;
                  result.attributes = transform_attributes( std::move( message.attributes));

                  result.payload.type = std::move( message.payload.type);
                  result.payload.data = std::move( message.payload.data);

                  return result;
               };

               auto enqueue( cli::message::payload::Message&& value)
               {
                  ipc::message::group::enqueue::Request result{ process::handle()};

                  result.message.payload.data = std::move( value.payload.data);
                  result.message.payload.type = std::move( value.payload.type);

                  return result;
               }

               auto enqueue( cli::message::queue::Message&& message)
               {
                  ipc::message::group::enqueue::Request result{ process::handle()};

                  result.message.attributes.properties = std::move( message.attributes.properties);
                  result.message.attributes.reply = std::move( message.attributes.reply);
                  result.message.attributes.available = message.attributes.available;
                  result.message.payload.data = std::move( message.payload.data);
                  result.message.payload.type = std::move( message.payload.type);

                  return result;
               }

            } // transform

            namespace legend
            {
               namespace list
               {
                  constexpr auto queues = R"(legend: list queues 
   name:
      name of the queue
   group:
      which group the queue is hosted on.
   rc:
      retry-count - the retry count of the queue. (error queues has 0 as retry count, hence has to be consumed to be removed)
   rd:
      retry-delay - the retry delay of the queue, if rolled backed available will be 'now + retry delay'.
   count:
      number of messages on the queue
   size:
      the current size of the queue, aggregated message sizes
   avg:
      average message size in the queue
   E:
      enqueue/dequeue enable; E = enqueue enabled, D = dequeue enabled.   
   EQ: 
      enqueued - total number of successfully enqueued messages on the queue (committed), over time.
      note: this also include moved and restored messages.
   DQ: 
      dequeued - total number of successfully dequeued messages on the queue (comitted), over time. 
      note: this also includes when a message is moved to an error queue if a retry-count is reached.   
   UC:
      number of currently uncommitted messages.
   last:
      the timestamp of the newest message on the queue, or has been on the queue if the queue is empty.
)";

               constexpr auto messages =  R"(legend: list messages 
   id:
      the id of the message
   S:
      the state of the message
         E: enqueued - not visible until commit
         C: committed - visible
         D: dequeued - not visible, removed on commit, back to state 'committed' if rolled back
   size:
      the size of the message
   trid:
      transaction trid
   rd:
      number of 'redeliver' (dequeues that has been rollbacked)
   type:
      type of the payload
   reply:
      the reply queue
   available:
      when the message is available for dequeue
   timestamp:
      when the message was enqueued
      
)";
                  namespace forward
                  {

                     constexpr auto groups = R"(legend: list-forward-groups
   alias:
      alias of the group
   pid:
      the pid of the process that is running the group
   S:
      the state of the group
         E: enabled
         D: disabled
   services:
      number of forward-services running within the group
   queues:
      number of forward-queues running within the group
   commits:
      accumulated number of commits for all forwards within the group
   rollbacks:
      accumulated number of rollbacks for all forwards within the group
   last:
      the last time one of the forwards did something
)";

                     constexpr auto services = R"(legend: list-forward-services
   alias:
      alias of the forward
   group:
      which group the forward is hosted on
   source:
      the queue to dequeue from
   target:
      the service to call
   reply:
      the queue to put the reply to, if any
   delay:
      delay of the reply message, duration until the message will be available for others to consume
   S:
      the state of the forward
         E: enabled
         D: disabled
   CI:
      configured 'instances'
   I:
      running 'instances'
   commits:
      number of commits the forward has performed
   rollbacks:
      number of rollbacks the forward has performed
   last:
      the last time the forward did something
)";

                     constexpr auto queues = R"(legend: list-forward-queues
   alias:
      alias of the forward
   group:
      which group the forward is hosted on
   source:
      the queue to dequeue from
   target:
      the queue to enqueue to
   delay:
      delay of the enqueued message, duration until the message will be available for others to consume
   S:
      the state of the forward
         E: enabled
         D: disabled
   CI:
      configured 'instances'
   I:
      running 'instances'
   commits:
      number of commits the forward has performed
   rollbacks:
      number of rollbacks the forward has performed
   last:
      the last time the forward did something
)";

                  } // forward
               } // list 

               auto option()
               {
                  auto invoke = []( const std::string& option)
                  {
                     if( option == "list-queues")
                        std::cout << legend::list::queues;
                     else if( option == "list-messages")
                        std::cout << legend::list::messages;
                     else if( option == "list-forward-groups")
                        std::cout << legend::list::forward::groups;
                     else if( option == "list-forward-services")
                        std::cout << legend::list::forward::services;
                     else if( option == "list-forward-queues")
                        std::cout << legend::list::forward::queues;
                  };

                  auto complete = []( auto& values, bool help) -> std::vector< std::string>
                  {     
                     return { "list-queues", "list-messages", "list-forward-groups", "list-forward-services", "list-forward-queues"};
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete,
                     { "--legend"},
                     R"(provide legend for the output for some of the options

to view legend for --list-queues use casual queue --legend list-queues, and so on.

use auto-complete to help which options has legends)"
                  };
               }

            } // legend 


            namespace list
            {
               namespace queues
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();

                        auto formatter = format::queues( state);

                        formatter.print( std::cout, algorithm::sort( state.queues));
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "-q", "--list-queues"},
                        R"(list information of all queues in current domain)"
                     };
                  }
                  
               } // queues

               namespace zombies
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();

                        auto formatter = format::queues( state);

                        formatter.print( std::cout, algorithm::sort( state.zombies));
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "-z", "--list-zombies"},
                        R"(list information of all zombie queues in current domain)"
                     };
                  }
                  
               } // queues

               namespace remote
               {
                  namespace queues
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           auto formatter = format::remote::queues( state);
                           formatter.print( std::cout, algorithm::sort( state.remote.queues));
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           argument::option::keys( {}, { "-r", "--list-remote"}),
                           R"(deprecated - use --list-instances)"
                        };
                     }
                  } // queues
               } // remote

               namespace queue::instances
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();
                        auto instances = normalize::instances( state);
                        auto formatter = format::queue::instances();
                        formatter.print( std::cout, instances);
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        { "-lqi", "--list-queue-instances"},
                        R"(list instances for all queues, including external instances)"
                     };
                  }
                  
               } // queue::instances

               namespace groups
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();
                        format::groups().print( std::cout, state.groups);
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        {  "-g", "--list-groups"},
                        "list information of all groups in current domain"
                     };
                  }
               } // groups

               namespace messages
               {
                  auto option()
                  {
                     auto invoke = []( const std::string& queue)
                     {
                        auto messages = call::messages( queue);
                        auto formatter = format::messages();
                        formatter.print( std::cout, messages);
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        complete::queues,
                        {  "-m", "--list-messages"},
                        "list information of all messages of the provided queue"
                     };
                  }
               } // messages

               namespace forward
               {
                  namespace services
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           format::forward::services( state).print( std::cout, state.forward.services);
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           {  "--list-forward-services"},
                           "list information of all service forwards"
                        };
                     }
                  } // services

                  namespace queues
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           format::forward::queues( state).print( std::cout, state.forward.queues);
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           {  "--list-forward-queues"},
                           "list information of all queue forwards"
                        };
                     }
                  } // queues

                  namespace groups
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           format::forward::groups( state).print( std::cout, state.forward.groups);
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           {  "--list-forward-groups"},
                           "list (aggregated) information of forward groups"
                        };
                     }
                  } // queues

               } // forward
            } // list

            namespace pipe
            {
               struct State
               {
                  cli::pipe::done::Scope done;
                  queue::ipc::message::lookup::Reply destination;
                  common::transaction::ID current;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( done);
                     CASUAL_SERIALIZE( destination);
                     CASUAL_SERIALIZE( current);
                  )
               };
               
            } // pipe

            namespace enqueue
            {
               auto create_enqueue_handle( const pipe::State& state)
               {
                  return [ &state]( auto& message)
                  {
                     Trace trace{ "queue::local::enqueue::create_enqueue_handle"};
                     log::line( verbose::log, "message: ", message);
                     log::line( verbose::log, "state: ", state);

                     auto request = local::transform::enqueue( std::move( message));

                     request.name = state.destination.name;
                     request.queue = state.destination.queue;
                     // use the explict transaction regardless.
                     request.trid = state.current;


                     auto reply = communication::ipc::call( state.destination.process.ipc, request);
                     cli::message::queue::message::ID id;
                     id.id = reply.id;

                     if( ! id.id)
                        code::raise::error( reply.code, "enqueue failed");
                     
                     cli::pipe::forward::message( id);
                  };
               }

               auto option()
               {
                  auto invoke = []( std::string name)
                  {
                     Trace trace{ "queue::local::enqueue::invoke"};

                     pipe::State state;
                     state.destination = queue::Lookup{ name, queue::Lookup::Action::enqueue}();

                     auto handler = cli::message::dispatch::create( 
                        cli::pipe::forward::handle::defaults(),
                        std::ref( state.done),
                        // handle Current, sets it our curren, and forward the message downstream
                        cli::pipe::transaction::handle::current( state.current),
                        // we will enqueue services replies and queue messages
                        cli::pipe::handle::payloads( create_enqueue_handle( state)));

                     // start the pump
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( cli::pipe::condition::done( state.done), handler, in);

                     // state.done dtor will send Done downstream

                  };

                  return argument::Option{
                     std::move( invoke),
                     complete::queues,
                     { "-e", "--enqueue"},
                     R"(enqueue buffer(s) to a queue from stdin

Assumes a conformant buffer(s)

Example:
cat somefile.bin | casual queue --enqueue <queue-name>

@note: part of casual-pipe)"
                  };

               }
               
            } // enqueue

            struct Empty : public std::runtime_error
            {
               using std::runtime_error::runtime_error;
            };

            namespace dequeue
            {
               auto dequeue_and_forward = []( const pipe::State& state, const std::optional< Uuid>& id)
               {
                  // we 'start' a new execution 'context'
                  common::execution::context::reset();

                  Trace trace{ "queue::local::dequeue::action"};
                  log::line( verbose::log, "state: ", state);

                  ipc::message::group::dequeue::Request request{ process::handle()};
                  request.name = state.destination.name;
                  request.queue = state.destination.queue;
                  request.trid = state.current;

                  if( id)
                     request.selector.id = *id;

                  log::line( verbose::log, "request: ", request);

                  if( auto reply = communication::ipc::call( state.destination.process.ipc, request))
                  {
                     log::line( verbose::log, "reply: ", reply);
                     
                     if( ! reply.message)
                        return false;

                     auto result = local::transform::message( std::move( *reply.message));
                     cli::pipe::forward::message( result);

                     return true;
                  }
                  return false;
               };

               auto option()
               {
                  auto invoke = []( std::string queue, const std::vector< Uuid>& ids)
                  {
                     Trace trace{ "queue::local::dequeue::invoke"};

                     pipe::State state;
                     state.destination = queue::Lookup{ std::move( queue), queue::Lookup::Action::dequeue}();

                     auto handler = cli::message::dispatch::create(
                        cli::pipe::forward::handle::defaults(),
                        // handle Current, sets it our curren, and forward the message downstream
                        cli::pipe::transaction::handle::current( state.current),
                        std::ref( state.done)
                     );

                     // consume the pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( cli::pipe::condition::done( state.done), handler, in);

                     if( std::empty( ids))
                     {
                        dequeue_and_forward( state, {});
                     }
                     else
                     {
                        for( auto& id : ids)
                           dequeue_and_forward( state, id);
                     }

                     // state.done dtor will send Done downstream
                  };

                  auto complete = []()
                  {
                     return []( auto, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<queue>", "[<id>..]"};

                        return local::queues();
                     };
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete(),
                     { "-d", "--dequeue"},
                     R"(dequeue message from a queue to `casual-pipe`

if id is absent the oldest available message is dequeued. 

Example:
casual queue --dequeue <queue> | <some other part in casual-pipe> | ... | <casual-pipe termination>
casual queue --dequeue <queue> <id> | <some other part in casual-pipe> | ... | <casual-pipe termination>
casual queue --dequeue <queue> <id> <id> <id> <id> | <some other part in casual-pipe> | ... | <casual-pipe termination>


@note: part of casual-pipe
)"

                  };
               }
            } // dequeue

            namespace consume
            {
               auto option()
               {
                  auto invoke = []( std::string queue, std::optional< platform::size::type> count)
                  {
                     Trace trace{ "queue::local::consume::invoke"};

                     pipe::State state;
                     state.destination = queue::Lookup{ std::move( queue), queue::Lookup::Action::dequeue}();

                     auto handler = cli::message::dispatch::create(
                        cli::pipe::forward::handle::defaults(),
                        cli::pipe::transaction::handle::current( state.current),
                        std::ref( state.done)
                     );

                     // consume the pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( cli::pipe::condition::done( state.done), handler, in);

                     auto remaining = count.value_or( std::numeric_limits< platform::size::type>::max());

                     while( remaining > 0 && dequeue::dequeue_and_forward( state, {}))
                        --remaining;

                     // state.done dtor will send Done downstream
                  };

                  return argument::Option{
                     std::move( invoke),
                     []( auto& values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<queue>", "<count>"};
                        
                        if( values.empty())
                           return local::queues();
                        else 
                           return { "<value>"};

                     },
                     { "--consume"},
                      R"(consumes up to `count` messages from the provided `queue` and send it downstream

Example:
casual queue --consume <queue-name> [<count>] | <some other part of casual-pipe> | ... | <casual-pipe-termination>

@note: part of casual-pipe
)"

                  };
               }
               
            } // consume

            namespace peek
            {
               auto option()
               {
                  auto invoke = [](const std::string& queue, const std::vector< queue::Message::id_type>& ids)
                  {
                     cli::pipe::done::Scope done;

                     auto handler = cli::message::dispatch::create(
                        cli::pipe::forward::handle::defaults(),
                        std::ref( done)
                     );

                     // consume the pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( cli::pipe::condition::done( done), handler, in);

                     algorithm::for_each( queue::peek::messages( queue, ids), []( auto& message)
                     {
                        cli::pipe::forward::message( local::transform::message( std::move( message)));
                     });

                     // done dtor will send Done downstream
                  };

                  auto complete = []( auto& values, bool help) -> std::vector< std::string>
                  { 
                     if( help) 
                        return { "<queue>", "[<id>]"};

                     if( values.empty())
                        return local::queues();

                     // complete on id
                     return algorithm::transform( 
                        call::messages( range::front( values)),
                        []( auto& message){ return uuid::string( message.id);});
                  };

                  return argument::Option{
                     std::move( invoke),
                     std::move( complete),
                     { "-p", "--peek"},
                     R"(peeks messages from the give queue and streams them to casual-pipe

Example:
casual queue --peek <queue-name> <id1> <id2> | <some other part of casual-pipe> | ... | <casual-pipe-termination>

@note: part of casual-pipe)"
                  };
               }
               
            } // peek

            namespace attributes
            {
               using namespace std::string_view_literals;
               constexpr auto names() noexcept { return array::make( "properties"sv, "reply"sv, "available"sv);}

               auto option()
               {
                  auto invoke = []( const std::vector< std::tuple< std::string, std::string>>& attributes)
                  {
                     auto handle_queue_message = [ &attributes]( cli::message::queue::Message& message)
                     {
                        for( auto& [ name, value] : attributes)
                        {
                           if( name == "properties")
                              message.attributes.properties = value;
                           else if( name == "reply")
                              message.attributes.reply = value;
                           else if( name == "available")
                              message.attributes.available = platform::time::point::type{ chronology::from::string( value)};
                           else
                              common::code::raise::error( common::code::casual::invalid_argument, "'", name, "' is not part of the valid set: ", attributes::names());
                        }

                        cli::pipe::forward::message( message);
                     };

                     auto handle_payload_message = [ handle_queue_message]( cli::message::payload::Message& message)
                     {
                        cli::message::queue::Message result;
                        result.correlation = message.correlation;
                        result.execution = message.execution;
                        result.payload = std::move( message.payload);

                        handle_queue_message( result);
                     };
                     
                    cli::pipe::done::Scope done;

                     auto handler = cli::message::dispatch::create(
                        cli::pipe::forward::handle::defaults(),
                        std::move( handle_queue_message),
                        std::move( handle_payload_message),
                        std::ref( done)
                     );

                     // consume from casual-pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( 
                        cli::pipe::condition::done( done), 
                        handler, in);

                     // done dtor will send Done downstream.
                  };

                  auto complete = []( auto& values, bool help) -> std::vector< std::string>
                  { 
                     if( help) 
                        return { "<attribute-name>", "<value>"};

                     if( values.size() % 2 == 0)
                        return algorithm::container::create< std::vector< std::string>>( attributes::names());   //{ "properties", "reply", "available"};
                     
                     if( ! values.empty() && range::back( values) == "available")
                        return { chronology::to::string( std::chrono::duration_cast< std::chrono::seconds>( platform::time::clock::type::now().time_since_epoch()))};

                     if( ! values.empty() && range::back( values) == "reply")
                        return local::queues();

                      return { "<value>"};
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     std::move( complete),
                     { "--attributes"},
                     R"(INCUBATION - adds or mutates queue message attributes on piped messages

@attention INCUBATION - might change during. or in between minor version.

Valid attributes:
* properties  | user defined string
* reply       | queue name
* available   | absolute time since epoch ([+]?<value>[h|min|s|ms|us|ns])+

Example:
`$ casual queue --dequeue a | casual queue --attributes reply a.reply properties foo | casual queue --enqueue a`

@note: Can be used to add queue attributes to a service reply_
@note: part of casual-pipe)"
                  };

               }
            } // attributes

            namespace restore
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     format::affected().print( std::cout, queue::restore::queue( queues));
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete::queues,
                     {  "--restore"},
                     R"(restores messages to queue

Messages will be restored to the queue they first was enqueued to (within the same queue-group)

Example:
casual queue --restore <queue-name>)"
                  };
               }
            } // restore

            namespace messages
            {
               namespace remove
               {
                  auto option( State& state)
                  {
                     auto invoke = [ &state]( const std::string& queue, Uuid id, std::vector< Uuid> ids)
                     {
                        ids.insert( std::begin( ids), std::move( id));
                        auto removed = queue::messages::remove( queue, ids, state.force);

                        algorithm::for_each( removed, []( auto& id)
                        {
                           std::cout << id << '\n';
                        });
                     };

                     auto complete = []( auto& values, bool help) -> std::vector< std::string>
                     { 
                        if( help) 
                           return { "<queue>", "<id>"};
                        
                        if( values.empty())
                           return local::queues();
                        
                        return { "<value>"};
                     };

                     return argument::Option{
                        std::move( invoke),
                        complete,
                        {  "--remove-messages"},
                        R"(removes specific messages from a given queue

if used with `--force true` messages will be removed regardless of state.)"
                     };
                  }
               } // remove

               namespace recovery
               {
                  using Directive = ipc::message::group::message::recovery::Directive;
                  namespace commit
                  {

                     auto option()
                     {
                        auto invoke = []( common::transaction::global::ID gtrid, std::vector< common::transaction::global::ID> gtrids)
                        {
                           gtrids.insert( std::begin( gtrids), std::move( gtrid));
                           auto commited = call::recover( std::move( gtrids), Directive::commit); 

                           algorithm::for_each( commited, []( auto& gtrid)
                           {
                              common::log::line( std::cout, gtrid);
                           });
                        };

                        auto complete = []( auto& values, bool help) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<gtrid>"};

                           return { "<value>"};
                        };

                        return argument::Option{
                           std::move( invoke),
                           complete,
                           {  "--recover-transactions-commit"},
                           R"(recover specific messages from a given queue with commit)"
                        };
                     }
                  } // commit

                  namespace rollback
                  {
                     auto option()
                     {
                        auto invoke = []( common::transaction::global::ID gtrid, std::vector< common::transaction::global::ID> gtrids)
                        {
                           gtrids.insert( std::begin( gtrids), std::move( gtrid));
                           auto rollbacked = call::recover( std::move( gtrids), Directive::rollback); 

                           algorithm::for_each( rollbacked, []( auto& gtrid)
                           {
                              common::log::line( std::cout, gtrid);
                           });
                        };

                        auto complete = []( auto& values, bool help) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<gtrid>"};

                           return { "<value>"};
                        };

                        return argument::Option{
                           std::move( invoke),
                           complete,
                           {  "--recover-transactions-rollback"},
                           R"(recover specific messages from a given queue with rollback)"
                        };
                     }
                  } // rollback
               } // recovery
            } // messages

            namespace clear
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     format::affected().print( std::cout, queue::clear::queue( queues));
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     complete::queues,
                     {  "--clear"},
                     R"(clears all messages from provided queues

Example:
casual queue --clear a b c)"
                  };
               }

            } // clear

            namespace forward::scale::aliases
            {
               auto option()
               {
                  auto invoke = []( std::vector< std::tuple< std::string, platform::size::type>> values)
                  {
                     auto aliases = algorithm::transform( values, []( auto& value)
                     {
                        if( std::get< 1>( value) < 0)
                           common::code::raise::error( common::code::casual::invalid_argument, "number of instances cannot be negative");
                              
                        manager::admin::model::scale::Alias result;
                        result.name = std::move( std::get< 0>( value));
                        result.instances = std::get< 1>( value);
                        return result;
                     });

                     serviceframework::service::protocol::binary::Call call;
                     call << CASUAL_NAMED_VALUE( aliases);
                     call( manager::admin::service::name::forward::scale::aliases);
                  };

                  auto complete = []( auto&& values, bool help) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<alias>", "<# instances>"};

                     if( range::size( values) % 2 == 1)
                        return { "<value>"};

                     auto get_alias = []( auto& forward){ return forward.alias;};

                     auto state = call::state();

                     auto result = algorithm::transform( state.forward.services, get_alias);
                     algorithm::transform( state.forward.services, std::back_inserter( result), get_alias);

                     return result;
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     complete,
                     {  "--forward-scale-aliases"},
                     R"(scales forward aliases to the requested number of instances

Example:
casual queue --forward-scale-aliases a 2 b 0 c 10)"
                  };
               };  

            } // forward::scale::aliases

            namespace metric::reset
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     call << CASUAL_NAMED_VALUE( queues);
                     auto reply = call( manager::admin::service::name::metric::reset);
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     complete::queues,
                     {  "--metric-reset"},
                     R"(resets metrics for the provided queues

if no queues are provided, metrics for all queues are reset.

Example:
casual queue --metric-reset a b)"
                  };
               }
                  
            } // metric::reset
            
            namespace information
            {
               auto call() -> std::vector< std::tuple< std::string, std::string>> 
               {
                  auto state = local::call::state();

                  auto accumulate = []( auto extract)
                  {
                     return [extract]( auto& queues)
                     {
                        decltype( extract( range::front( queues))) initial{};
                        return algorithm::accumulate( queues, initial, [extract]( auto count, auto& queue){ return count + extract( queue);});
                     };
                  };

                  auto max = []( auto extract)
                  {
                     return [extract]( auto& range)
                     {
                        decltype( extract( range::front( range))) initial{};
                        return algorithm::accumulate( range, initial, [extract]( auto value, auto& element){ return std::max( value, extract( element));});
                     };
                  };

                  auto message_count = []( auto& queue){ return queue.count;};
                  auto message_size = []( auto& queue){ return queue.size;};

                  auto metric_enqueued = []( auto& queue){ return queue.metric.enqueued;};
                  auto metric_dequeued = []( auto& queue){ return queue.metric.dequeued;};

                  auto split = algorithm::partition( state.queues, []( auto& queue){ return queue.type() == decltype( queue.type())::queue;});
                  auto queues = std::get< 0>( split);
                  auto errors = std::get< 1>( split);

                  auto commit_count = []( auto& forward){ return forward.metric.commit.count;};
                  auto commit_last = []( auto& forward){ return forward.metric.commit.last;};
                  auto rollback_count = []( auto& forward){ return forward.metric.rollback.count;};
                  auto rollback_last = []( auto& forward){ return forward.metric.rollback.last;};

                  return {
                     { "queue.manager.group.count", string::compose( state.groups.size())},
                     { "queue.manager.queue.count", string::compose( queues.size())},
                     { "queue.manager.queue.message.count", string::compose( accumulate( message_count)( queues))},
                     { "queue.manager.queue.message.size", string::compose( accumulate( message_size)( queues))},
                     { "queue.manager.queue.metric.enqueued", string::compose( accumulate( metric_enqueued)( queues))},
                     { "queue.manager.queue.metric.dequeued", string::compose( accumulate( metric_dequeued)( queues))},
                     { "queue.manager.error.queue.message.count", string::compose( accumulate( message_count)( errors))},
                     { "queue.manager.error.queue.message.size", string::compose( accumulate( message_size)( errors))},
                     { "queue.manager.error.queue.metric.enqueued", string::compose( accumulate( metric_enqueued)( errors))},
                     { "queue.manager.error.queue.metric.dequeued", string::compose( accumulate( metric_dequeued)( errors))},
                     { "queue.manager.remote.domain.count", string::compose( state.remote.domains.size())},
                     { "queue.manager.remote.queue.count", string::compose( state.remote.queues.size())},
                     { "queue.manager.forward.group.count", string::compose( state.forward.groups.size())},
                     { "queue.manager.forward.services.count", string::compose( state.forward.services.size())},
                     { "queue.manager.forward.services.metric.commit.count", string::compose( accumulate( commit_count)( state.forward.services))},
                     { "queue.manager.forward.services.metric.commit.last", string::compose( max( commit_last)( state.forward.services))},
                     { "queue.manager.forward.services.metric.rollback.count", string::compose( accumulate( rollback_count)( state.forward.services))},
                     { "queue.manager.forward.services.metric.rollback.last", string::compose( max( rollback_last)( state.forward.services))},
                     { "queue.manager.forward.queues.count", string::compose( state.forward.queues.size())},
                     { "queue.manager.forward.queues.metric.commit.count", string::compose( accumulate( commit_count)( state.forward.queues))},
                     { "queue.manager.forward.queues.metric.commit.last", string::compose( max( commit_last)( state.forward.queues))},
                     { "queue.manager.forward.queues.metric.rollback.count", string::compose( accumulate( rollback_count)( state.forward.queues))},
                     { "queue.manager.forward.queues.metric.rollback.last", string::compose( max( rollback_last)( state.forward.queues))},
                  };
               };

               auto option()
               {
                  auto invoke = []()
                  {
                     terminal::formatter::key::value().print( std::cout, information::call());
                  };

                  return argument::Option{
                     std::move( invoke),
                     {  "--information"},
                     "collect aggregated information about queues in this domain"};
                  }

            } // information

            namespace force
            {
               auto option( State& state)
               {
                  auto complete = []( auto&&, auto) -> std::vector< std::string>
                  {
                     return { "true", "false"};
                  };

                  return argument::Option{
                     std::tie( state.force),
                     complete,
                     { "--force"},
                     R"(force removal of message regardless of state (default: false)

used in conjunction with `--remove-messages`)"
                  };
               }
            } // force

         } // <unnamed>
      } // local

      namespace manager
      {
         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  return argument::Group{ [](){}, { "queue"}, "queue related administration",
                     local::list::queues::option(),
                     local::list::zombies::option(),
                     local::list::queue::instances::option(),
                     local::list::groups::option(),
                     local::list::messages::option(),
                     local::list::forward::services::option(),
                     local::list::forward::queues::option(),
                     local::list::forward::groups::option(),
                     local::restore::option(),
                     local::enqueue::option(),
                     local::dequeue::option(),
                     local::peek::option(),
                     local::consume::option(),
                     local::attributes::option(),
                     local::clear::option(),
                     local::messages::remove::option( state),
                     local::force::option( state),
                     local::messages::recovery::commit::option(),
                     local::messages::recovery::rollback::option(),
                     local::forward::scale::aliases::option(),
                     local::metric::reset::option(),
                     local::legend::option(),
                     local::information::option(),
                     casual::cli::state::option( &local::call::state),
                     local::list::remote::queues::option(),
                  };
               }

               local::State state;
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }

            std::vector< std::tuple< std::string, std::string>> cli::information() &
            {
               return local::information::call();
            }
            
         } // admin
      } // manager
   } // queue
} // casual



