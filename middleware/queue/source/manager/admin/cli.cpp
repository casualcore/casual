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

#include "casual/argument.h"
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

#include "service/protocol/call.h"

#include <iostream>
#include <string_view>

namespace casual
{
   using namespace common;

   namespace queue::manager::admin::cli
   {

      namespace local
      {
         namespace
         {
            namespace global
            {
               struct
               {
                  casual::transaction::ID trid;
               } state;

            } // global

            namespace normalize
            {
               std::string timestamp( const common::chronology::time_point& time)
               {
                  if( time != common::chronology::empty())
                     return common::chronology::utc::offset( time);

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

               struct Instance
               {
                  std::string queue;
                  instance::State state = instance::State::internal;
                  process::Handle process;
                  std::string alias;
                  std::string description;
                  platform::size::type order{};

                  inline friend auto operator <=> ( const Instance& lhs, const Instance& rhs) = default;
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
                  casual::service::protocol::binary::Call call;
                  return call( manager::admin::service::name::state).extract< manager::admin::model::State>();
               }

               std::vector< manager::admin::model::Message> messages( std::string_view queue)
               {
                  casual::service::protocol::binary::Call call;
                  return call( manager::admin::service::name::messages::list, queue).extract< std::vector< manager::admin::model::Message>>();
               }

               std::vector< common::transaction::global::ID> recover( const std::vector< common::transaction::global::ID>& gtrids,
                  ipc::message::group::message::recovery::Directive directive)
               {
                  using Call = casual::service::protocol::binary::Call;
                  return Call{}( manager::admin::service::name::recover,
                     std::move( gtrids),
                     std::move( directive)).extract< std::vector< common::transaction::global::ID>>();
               }
            } // call

            namespace lookup
            {
               auto queue( std::string_view queue, queue::Lookup::Action action)
               {
                  auto result = queue::Lookup{ queue, action}();

                  if( ! result.process.ipc)
                     code::raise::error( code::queue::no_queue, "failed to lookup queue: ", queue);

                  return result;
               }
            }


            namespace format
            {
               using duration_type = std::chrono::duration< double>;

               template< typename P = std::identity>
               auto column_alias( P projection = {})
               {
                  return terminal::format::column( "alias", [projection]( auto& value){ return projection( value).alias;}, terminal::color::yellow);
               };

               auto column_pid ()
               {
                  return terminal::format::column( "pid", []( auto& value){ return value.process.pid;}, terminal::color::white);
               };

               template< typename P = std::identity>
               auto column_source( P projection = {})
               {
                  return terminal::format::column( "source", [projection]( auto& value){ return projection( value).source;}, terminal::color::white);
               };

               auto column_group( auto& groups)
               {
                  return terminal::format::column( "group", [ &groups]( auto& value)
                  { 
                     if( auto found = algorithm::find( groups, value.group))
                        return found->alias;
                     return std::to_string( value.group.value());
                  }, terminal::color::white);
               };

               auto column_enabled()
               {
                  // todo: is this the best title?
                  return terminal::format::column( "S", []( auto& value)
                  {
                     return value.enabled ? "E" : "D";
                  });
               }

               auto column_configured_instances()
               { 
                  return terminal::format::column( "CI", []( auto& value)
                  { 
                     return value.instances.configured;
                  }, terminal::color::white, terminal::format::Align::right);
               }

               auto column_running_instances()
               { 
                  return terminal::format::column( "I", []( auto& value)
                  { 
                     return value.instances.running;
                  }, terminal::color::white, terminal::format::Align::right);
               }

               auto column_metric_commit()
               {
                  return terminal::format::column( "commits", []( auto& value)
                  { 
                     return value.metric.commit.count;
                  }, terminal::color::cyan, terminal::format::Align::right);
               }

               auto column_metric_rollback()
               {
                  return terminal::format::column( "rollbacks", []( auto& value)
                  { 
                     return value.metric.rollback.count;
                  }, terminal::color::cyan, terminal::format::Align::right);
               }

               auto column_metric_last()
               {
                  return terminal::format::column( "last", []( auto& value)
                  {
                     return local::normalize::timestamp( std::max( value.metric.commit.last, value.metric.rollback.last));
                  }, terminal::color::blue);
               };

               namespace aggregated
               {
                  auto column_queues_count = []( const auto& queues)
                  {
                     return terminal::format::column( "queues", [&queues]( auto& value)
                     { 
                        return algorithm::count( queues, value.process.pid);

                     }, terminal::color::blue, terminal::format::Align::right);
                  };

                  namespace detail
                  {
                     auto aggregate_metric( auto pid, const auto&... ranges)
                     {
                        auto accumulate_range = [pid]( const auto& range)
                        {
                           using metric_type = decltype( common::range::front( range).metric);

                           return algorithm::accumulate( range, metric_type{}, [pid]( auto result, auto& value)
                           {
                              if( value != pid)
                                 return result;

                              return result + value.metric;
                           });
                        };

                        return ( accumulate_range( ranges) + ...);
                     }
                  } // detail

                  auto column_commit( const auto&... ranges)
                  {
                     return terminal::format::column( "commits", [&ranges...]( auto& value)
                     {
                        auto metric = detail::aggregate_metric( value.process.pid, ranges...);

                        return metric.commit.count;
                     }, terminal::color::cyan, terminal::format::Align::right);
                  };

                  auto column_rollback( const auto&... ranges)
                  {
                     return terminal::format::column( "rollbacks", [&ranges...]( auto& value)
                     { 
                        auto metric = detail::aggregate_metric( value.process.pid, ranges...);

                        return metric.rollback.count;
                     }, terminal::color::cyan, terminal::format::Align::right);
                  };

                  auto column_last( const auto&... ranges)
                  {
                     return terminal::format::column( "last", [&ranges...]( auto& value)
                     {
                        auto metric = detail::aggregate_metric( value.process.pid, ranges...);

                        return local::normalize::timestamp( std::max( metric.commit.last, metric.rollback.last));
                     }, terminal::color::blue);
                  };
                  
               } // aggregated

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

               auto messages( auto&& messages)
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

                  terminal::format::print( messages,
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

               auto groups( auto&& groups)
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

                  terminal::format::print( groups,
                     terminal::format::column( "alias", format_alias, terminal::color::yellow),
                     terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "ipc", format_ipc, terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "queuebase", format_queuebase, terminal::color::cyan),
                     terminal::format::column( "size", format_size, terminal::format::Align::right),
                     terminal::format::column( "capacity", format_capacity, terminal::format::Align::right)
                  );
               }

               auto affected( auto&& affected)
               {
                  auto format_name = []( auto& value) { return value.queue;};

                  terminal::format::print( affected,
                     terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::right),
                     terminal::format::column( "count", std::mem_fn( &queue::restore::Affected::count), terminal::color::green)
                  );
               }


               auto queues( const manager::admin::model::State& state, auto&& queues)
               {
                  auto format_retry_delay = []( auto& queue)
                  {
                     return std::chrono::duration_cast< format::duration_type>( queue.retry.delay).count();
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
                     terminal::format::print( queues,
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
                     terminal::format::print( queues,
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
                  auto queues( auto&& queues)
                  {
                     auto format_pid = [&]( auto& queue){ return queue.process.pid;};

                     auto format_name = []( auto& queue){ return queue.name;};
         
         
                     terminal::format::print( queues,
                        terminal::format::column( "name", format_name, terminal::color::yellow),
                        terminal::format::column( "pid", format_pid, common::terminal::color::blue)
                     );
                  }
                  
               } // remote

               namespace queue
               {
                  auto instances( auto&& instances)
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
         
                     terminal::format::print( instances,
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
                  auto services( const manager::admin::model::State& state, auto&& services)
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

                           return std::to_string( std::chrono::duration_cast< format::duration_type>( forward.reply.value().delay).count());
                        }, terminal::color::cyan, terminal::format::Align::right);
                     };

                     if( ! terminal::output::directive().porcelain())
                     {
                        terminal::format::print( services,
                           local::format::column_alias(),
                           local::format::column_group( state.forward.groups),
                           local::format::column_source(),
                           column_target(),
                           column_reply_name(),
                           column_reply_delay(),
                           local::format::column_enabled(),
                           local::format::column_configured_instances(),
                           local::format::column_running_instances(),
                           local::format::column_metric_commit(),
                           local::format::column_metric_rollback(),
                           local::format::column_metric_last()
                        );
                     }
                     else
                     {
                        terminal::format::print( services,
                           local::format::column_alias(),
                           local::format::column_group( state.forward.groups),
                           local::format::column_source(),
                           column_target(),
                           column_reply_name(),
                           column_reply_delay(),
                           local::format::column_configured_instances(),
                           local::format::column_running_instances(),
                           local::format::column_metric_commit(),
                           local::format::column_metric_rollback(),
                           local::format::column_metric_last(),
                           local::format::column_enabled()
                        );
                     }
                  }
   
                  auto queues( const manager::admin::model::State& state, auto&& queues)
                  {
                     auto column_target = []()
                     {
                        return terminal::format::column( "target", []( auto& forward){ return forward.target.queue;}, terminal::color::white);
                     };

                     auto column_target_delay = []()
                     {
                        return terminal::format::column( "delay", []( auto& forward)
                        { 
                           return std::chrono::duration_cast< format::duration_type>( forward.target.delay).count();
                        }, terminal::color::white, terminal::format::Align::right);
                     };

                     if( ! terminal::output::directive().porcelain())
                     {
                        terminal::format::print( queues,
                           local::format::column_alias(),
                           local::format::column_group( state.forward.groups),
                           local::format::column_source(),
                           column_target(),
                           column_target_delay(),
                           local::format::column_enabled(),
                           local::format::column_configured_instances(),
                           local::format::column_running_instances(),
                           local::format::column_metric_commit(),
                           local::format::column_metric_rollback(),
                           local::format::column_metric_last()
                        );
                     }
                     else
                     {
                        terminal::format::print( queues,
                           local::format::column_alias(),
                           local::format::column_group( state.forward.groups),
                           column_source(),
                           column_target(),
                           column_target_delay(),
                           local::format::column_configured_instances(),
                           local::format::column_running_instances(),
                           local::format::column_metric_commit(),
                           local::format::column_metric_rollback(),
                           local::format::column_metric_last(),
                           local::format::column_enabled()
                        );
                     }
                  }

                  auto groups( const manager::admin::model::State& state, auto&& groups)
                  {
                     auto column_services = [&state]()
                     {
                        return terminal::format::column( "services", [&state]( auto& group)
                        { 
                           return algorithm::count( state.forward.services, group.process.pid);

                        }, terminal::color::blue, terminal::format::Align::right);
                     };

                     if( ! terminal::output::directive().porcelain())
                     {
                        terminal::format::print( groups,
                           local::format::column_alias(),
                           local::format::column_pid(),
                           column_services(),
                           local::format::aggregated::column_queues_count( state.forward.queues),
                           local::format::aggregated::column_commit(  state.forward.services, state.forward.queues),
                           local::format::aggregated::column_rollback( state.forward.services, state.forward.queues),
                           local::format::aggregated::column_last( state.forward.services, state.forward.queues)
                        );
                     }
                     else
                     {
                        terminal::format::print( groups,
                           local::format::column_alias(),
                           local::format::column_pid(),
                           column_services(),
                           local::format::aggregated::column_queues_count( state.forward.queues),
                           local::format::aggregated::column_commit( state.forward.services, state.forward.queues),
                           local::format::aggregated::column_rollback( state.forward.services, state.forward.queues),
                           local::format::aggregated::column_last( state.forward.services, state.forward.queues)
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
               auto queues = []( bool help, auto values) -> std::vector< std::string>
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
                     casual::cli::message::queue::message::Attributes result;
                     result.properties = std::move( value.properties);
                     result.reply = std::move( value.reply);
                     result.available = value.available;
                     return result;
                  };

                  casual::cli::message::queue::Message result;
                  result.id = message.id;
                  result.attributes = transform_attributes( std::move( message.attributes));

                  result.payload = std::move( message.payload);

                  return result;
               };

               auto enqueue( casual::cli::message::payload::Message&& value)
               {
                  ipc::message::group::enqueue::Request result{ process::handle()};

                  result.message.payload = std::move( value.payload);

                  return result;
               }

               auto enqueue( casual::cli::message::queue::Message&& message)
               {
                  ipc::message::group::enqueue::Request result{ process::handle()};

                  result.message.attributes.properties = std::move( message.attributes.properties);
                  result.message.attributes.reply = std::move( message.attributes.reply);
                  result.message.attributes.available = message.attributes.available;
                  
                  result.message.payload = std::move( message.payload);

                  return result;
               }

            } // transform

            namespace list
            {
               namespace queues
               {
                  constexpr auto legend = R"(
output columns:
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

                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();

                        format::queues( state, algorithm::sort( state.queues));
                     };

                     return argument::Option{
                        std::move( invoke),
                        argument::option::Names{ { "-lq", "--list-queues"}, { "-q"}},
                        { "list information of all queues in current domain", legend}
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

                        format::queues( state, algorithm::sort( state.zombies));

                     };

                     return argument::Option{
                        std::move( invoke),
                        argument::option::Names{ { "-lz", "--list-zombies"}, { "-z"}},
                        R"(list information of all zombie queues in current domain)"
                     };
                  }
                  
               } // queues

               namespace queue::instances
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();
                        format::queue::instances( normalize::instances( state));
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        {{ "-lqi", "--list-queue-instances"}},
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
                        format::groups( state.groups);
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        {{ "-lg", "--list-groups"}, { "-g"}},
                        "list information of all groups in current domain"
                     };
                  }
               } // groups

               namespace messages
               {
                  constexpr auto legend =  R"(
output columns:
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
                  auto option()
                  {
                     auto invoke = []( const std::string& queue)
                     {
                        auto messages = call::messages( queue);
                        format::messages( messages);
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        complete::queues,
                        {{  "-lm", "--list-messages"}, { "-m"}},
                        { "list information of all messages of the provided queue", legend}
                     };
                  }
               } // messages

            } // list

            namespace forward
            {
               namespace list
               {
                  namespace services
                  {
                     namespace detail
                     {                     
                        auto option( argument::option::Names names, argument::option::Description description)
                        {
                           auto invoke = []()
                           {
                              auto state = call::state();
                              format::forward::services( state, state.forward.services);
                           };
                           
                           return argument::Option{
                              std::move( invoke),
                              std::move( names), 
                              std::move( description)
                           };
                        }
                     } // detail

                     // "list information of all service forwards"

                    constexpr auto legend = R"(
output columns:
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

                     auto option()
                     {
                        return detail::option( {{ "-ls", "--list-services"}}, { "list information of all service forwards", legend});
                     }

                     auto deprecated_option()
                     {
                        return detail::option( { {}, {  "-lfs", "--list-forward-services"}}, { "@deprecated: use `casual queue forward --list-services` instead"});
                     }

                  } // services

                  namespace queues
                  {
                     namespace detail
                     {                     
                        auto option( argument::option::Names names, argument::option::Description description)
                        {
                           auto invoke = []()
                           {
                              auto state = call::state();
                              format::forward::queues( state, state.forward.queues);
                           };
                           
                           return argument::Option{
                              std::move( invoke),
                              std::move( names),
                              std::move( description)
                           };
                        }
                     } // detail


                     constexpr auto legend = R"(
output columns:
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

                     auto option()
                     {
                        return detail::option( {{ "-lq", "--list-queues"}}, { "list information of all queue forwards", legend});
                     }

                     auto deprecated_option()
                     {
                        return detail::option( {{}, { "-lfq", "--list-forward-queues"}}, { "@deprecated: use `casual queue forward --list-queues` instead"});
                     }

                  } // queues

                  namespace groups
                  {
                     namespace detail
                     {                     
                        auto option( argument::option::Names names, argument::option::Description description)
                        {
                           auto invoke = []()
                           {
                              auto state = call::state();
                              format::forward::groups( state, state.forward.groups);
                           };
                           
                           return argument::Option{
                              std::move( invoke),
                              std::move( names), //{ "-lfg", "--list-forward-groups"},
                              std::move( description)
                           };
                        }
                     } // detail

                     constexpr auto legend = R"(
output columns:
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

                     auto option()
                     {
                        return detail::option( {{ "-lg", "--list-groups"}}, { "list (aggregated) information of forward groups", legend });
                     }

                     auto deprecated_option()
                     {
                        return detail::option( {{}, { "-lfg", "--list-forward-groups"}}, { "@deprecated: use `casual queue forward --list-groups` instead"});
                     }

                  } // groups
               } // list

               namespace scale::aliases
               {
                  namespace detail
                  {
                     auto option( argument::option::Names names, std::string description)
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

                           casual::service::protocol::binary::Call call;
                           call << CASUAL_NAMED_VALUE( aliases);
                           call( manager::admin::service::name::forward::scale::aliases);
                        };

                        auto complete = []( bool help, auto values) -> std::vector< std::string>
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
                           std::move( names), // { "--forward-scale-aliases"},
                           std::move( description)
                        };
                     };  
                  } // detail

                  auto option()
                  {
                     return detail::option( {{ "--scale-aliases"}}, R"(scales forward aliases to the requested number of instances

   Example:
   casual queue --scale-aliases a 2 b 0 c 10)");
                  }

                  auto deprecated_option()
                  {
                     return detail::option( {{}, { "--forward-scale-aliases"}}, "deprecated: use`casual queue forward --scale-aliases` instead");
                  }

               } // scale::aliases

               auto option()
               {
                  return argument::Option{
                     [](){},
                     {{ "forward"}},
                     R"(subcommand for forward)"
                  }(
                     {
                        list::services::option(),
                        list::queues::option(),
                        list::groups::option(),
                        scale::aliases::option(),
                     },
                     argument::cardinality::one()
                  );
               }
               
            } // forward


            namespace fanout
            {
               namespace list
               {
                  namespace groups
                  {
                     namespace detail
                     {
                        auto format( const admin::model::Fanout& fanout)
                        {
                           terminal::format::print( fanout.groups,
                              local::format::column_alias(),
                              local::format::column_pid(),
                              local::format::aggregated::column_queues_count( fanout.queues),
                              local::format::aggregated::column_commit( fanout.queues),
                              local::format::aggregated::column_rollback( fanout.queues),
                              local::format::aggregated::column_last( fanout.queues)
                           );
                        }
                        
                     } // detail

                     constexpr auto legend = R"(
output columns:
   alias:
      alias of the fanout group
   pid:
      the pid of the process that is running the fanout group
   queues:
      number of queue fanouts attached to the fanout group
   commits:
      accumulated number of commits for all queues within the fanout group
   rollbacks:
      accumulated number of rollbacks for all queues within the fanout group
   last:
      the last time one of the group queues committed or rollbacked a message
)";

                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           detail::format( state.fanout);
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           {{ "--list-groups"}},
                           { "list information of all fanout groups in current domain", legend}
                        };
                     }
                     
                  } // groups

                  namespace queues
                  {
                     namespace detail
                     {
                        auto format( const admin::model::Fanout& fanout)
                        {
                           auto target_count = []( auto& queue){ return queue.targets.size();};
                          
                           terminal::format::print( fanout.queues,
                              local::format::column_alias(),
                              local::format::column_group( fanout.groups),
                              local::format::column_source(),
                              terminal::format::column( "T#", target_count, terminal::color::blue),
                              local::format::column_enabled(),
                              local::format::column_configured_instances(),
                              local::format::column_running_instances(),
                              local::format::column_metric_commit(),
                              local::format::column_metric_rollback(),
                              local::format::column_metric_last()

                           );
                        }
                     } // detail

                     constexpr auto legend = R"(
output columns:
   alias:
      alias of the fanout queue
   group:
      which fanout group the queue is attached to
   source:
      the queue to dequeue from
   T#:
      number of target queues attached to the fanout queue
   S:
      the state of the fanout queue
         E: enabled
         D: disabled
   CI:
      configured 'instances'
   I:
      running 'instances'
   commits:
      number of commits the fanout queue has performed
   rollbacks:
      number of rollbacks the fanout queue has performed
   last:
      the last time the fanout queue did something
)";

                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           detail::format( state.fanout);
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           {{ "--list-queues"}},
                           { "list all fanout destinations in current domain", legend}
                        };

                     }
                  } // queues

                  namespace targets
                  {
                     namespace detail
                     {
                        auto format( const admin::model::Fanout& fanout)
                        {
                           // just a holder to flatten the structure a bit
                           struct Target
                           {
                              const admin::model::fanout::Queue* queue;
                              const admin::model::fanout::Queue::Target* target;
                           };

                           auto project_queue = []( const Target& target) -> const admin::model::fanout::Queue& { return *target.queue;};


                           std::vector< Target> targets;
                           for( auto& queue : fanout.queues)
                              for( auto& target : queue.targets)
                                 targets.push_back( Target{ &queue, &target});

                           auto target_name = []( const Target& target) { return target.target->queue;};
                           auto target_delay = []( const Target& target) 
                           { 
                              return std::chrono::duration_cast< format::duration_type>( target.target->delay).count();
                           };

                           terminal::format::print( targets,
                              local::format::column_alias( project_queue),
                              local::format::column_source( project_queue),
                              terminal::format::column( "target", target_name, terminal::color::white),
                              terminal::format::column( "delay", target_delay, common::terminal::color::blue, terminal::format::Align::right)
                           );

                        }
                     } // detail

                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           detail::format( state.fanout);
                        };

                        return argument::Option{
                           std::move( invoke),
                           {{ "--list-targets"}},
                           R"(list all fanout targets in current domain)"
                        };
                     }
                     
                  } // targets
                  
               } // list

               auto option()
               {
                  return argument::Option{
                     [](){},
                     {{ "fanout"}},
                     R"(subcommand for fanout)"
                  }(
                     {
                        list::groups::option(),
                        list::queues::option(),
                        list::targets::option(),
                     },
                     argument::cardinality::one()
                  );
               }
               
            } // fanout

            namespace pipe
            {
               struct State
               {
                  casual::cli::pipe::done::Scope done;
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
                     log::debug( "message: ", message);
                     log::debug( "state: ", state);

                     auto request = local::transform::enqueue( std::move( message));

                     request.name = state.destination.name;
                     request.queue = state.destination.queue;
                     // use the explict transaction regardless.
                     request.trid = state.current;


                     auto reply = communication::ipc::call( state.destination.process.ipc, request);
                     casual::cli::message::queue::message::ID id;
                     id.id = reply.id;

                     if( ! id.id)
                        code::raise::error( reply.code, "enqueue failed");
                     
                     casual::cli::pipe::forward::message( id);
                  };
               }

               auto option()
               {
                  auto invoke = []( std::string queue)
                  {
                     Trace trace{ "queue::local::enqueue::invoke"};

                     pipe::State state;
                     state.destination = lookup::queue( queue, queue::Lookup::Action::enqueue);

                     auto handler = casual::cli::message::dispatch::create( 
                        casual::cli::pipe::forward::handle::defaults(),
                        std::ref( state.done),
                        // handle Current, sets it our curren, and forward the message downstream
                        casual::cli::pipe::transaction::handle::current( state.current),
                        // we will enqueue services replies and queue messages
                        casual::cli::pipe::handle::payloads( create_enqueue_handle( state)));

                     // start the pump
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( casual::cli::pipe::condition::done( state.done), handler, in);

                     // state.done dtor will send Done downstream

                  };

                  constexpr auto description = R"(enqueue buffer(s) to a queue from stdin

Assumes conformant buffer(s)

@note: part of casual-pipe
)";
                  constexpr auto extended = R"(
Examples:

   cat somefile.bin | casual queue --enqueue a

   casual transaction --begin \
      | casual queue --dequeue a \
      | casual queue --enqueue b \
      | casual transaction --commit

)";

                  return argument::Option{
                     std::move( invoke),
                     complete::queues,
                     {{ "-e", "--enqueue"}},
                     { description, extended}
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
                  log::debug( "state: ", state);

                  ipc::message::group::dequeue::Request request{ process::handle()};
                  request.name = state.destination.name;
                  request.queue = state.destination.queue;
                  request.trid = state.current;

                  if( id)
                     request.selector.id = *id;

                  log::debug( "request: ", request);

                  if( auto reply = communication::ipc::call( state.destination.process.ipc, request))
                  {
                     log::debug( "reply: ", reply);
                     
                     if( ! reply.message)
                        return false;

                     auto result = local::transform::message( std::move( *reply.message));
                     casual::cli::pipe::forward::message( result);

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
                     state.destination = lookup::queue( queue, queue::Lookup::Action::enqueue);

                     auto handler = casual::cli::message::dispatch::create(
                        casual::cli::pipe::forward::handle::defaults(),
                        // handle Current, sets it our curren, and forward the message downstream
                        casual::cli::pipe::transaction::handle::current( state.current),
                        std::ref( state.done)
                     );

                     // consume the pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( casual::cli::pipe::condition::done( state.done), handler, in);

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
                     return []( bool help, auto) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<queue>", "[<id>..]"};

                        return local::queues();
                     };
                  };

                  constexpr auto description = R"(dequeue message from a queue to `casual-pipe`

if id is absent the oldest available message is dequeued.

@note: part of casual-pipe
)";

                  constexpr auto extended = R"(
Examples:

   casual queue --dequeue a > somefile.bin

   casual queue --dequeue a 123e4567e89b12d3a456426614174000 \
      | casual queue --enqueue b

   casual transaction --begin \
      | casual queue --dequeue a \
      | casual queue --enqueue b \
      | casual transaction --commit                  
)";

                  return argument::Option{
                     std::move( invoke),
                     complete(),
                     {{ "-d", "--dequeue"}},
                     { description, extended}
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
                     state.destination = lookup::queue( queue, queue::Lookup::Action::enqueue);

                     auto handler = casual::cli::message::dispatch::create(
                        casual::cli::pipe::forward::handle::defaults(),
                        casual::cli::pipe::transaction::handle::current( state.current),
                        std::ref( state.done)
                     );

                     // consume the pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( casual::cli::pipe::condition::done( state.done), handler, in);

                     auto remaining = count.value_or( std::numeric_limits< platform::size::type>::max());

                     while( remaining > 0 && dequeue::dequeue_and_forward( state, {}))
                        --remaining;

                     // state.done dtor will send Done downstream
                  };

                  auto complete = []( bool help, auto values) -> std::vector< std::string>
                  {
                     if( help)
                        return { "<queue>", "<count>"};

                     if( values.empty())
                        return local::queues();
                     else 
                        return { "<value>"};
                  };

                  constexpr auto description = R"(consumes messages from the provided `queue` and send it downstream

@note: part of casual-pipe
)";

                  constexpr auto extended = R"(
Example:

   casual queue --consume a > /dev/null

   casual queue --consume a 10 | casual queue --enqueue b

   casual transaction --begin \
      | casual queue --consume a \
      | casual queue --enqueue b \
      | casual transaction --commit
)";

                  return argument::Option{
                     std::move( invoke),
                     std::move( complete),
                     {{ "--consume"}},
                     { description, extended}
                  };
               }
               
            } // consume

            namespace peek
            {
               auto option()
               {
                  auto invoke = [](const std::string& queue, const std::vector< queue::Message::id_type>& ids)
                  {
                     casual::cli::pipe::done::Scope done;

                     auto handler = casual::cli::message::dispatch::create(
                        casual::cli::pipe::forward::handle::defaults(),
                        std::ref( done)
                     );

                     // consume the pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);

                     algorithm::for_each( queue::peek::messages( queue, ids), []( auto& message)
                     {
                        casual::cli::pipe::forward::message( local::transform::message( std::move( message)));
                     });

                     // done dtor will send Done downstream
                  };

                  auto complete = []( bool help, auto values) -> std::vector< std::string>
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

                  constexpr auto description = R"(peeks messages from the given queue and send it downstream
                  
@note: part of casual-pipe
)";
                  constexpr auto extended = R"(
Example:

   casual queue --peek a 123e4567e89b12d3a456426614174000

   casual queue --peek a | casual queue --enqueue b

)";

                  return argument::Option{
                     std::move( invoke),
                     std::move( complete),
                     {{ "-p", "--peek"}},
                     { description, extended}
                  };
               }
               
            } // peek

            namespace attributes
            {
               namespace deprecated
               {
                  using namespace std::string_view_literals;
                  constexpr auto names() noexcept { return array::make( "properties"sv, "reply"sv, "available"sv);}

                  auto option()
                  {
                     auto invoke = []( const std::vector< std::tuple< std::string, std::string>>& attributes)
                     {
                        auto handle_queue_message = [ &attributes]( casual::cli::message::queue::Message& message)
                        {
                           for( auto& [ name, value] : attributes)
                           {
                              if( name == "properties")
                                 message.attributes.properties = value;
                              else if( name == "reply")
                                 message.attributes.reply = value;
                              else if( name == "available")
                                 message.attributes.available = common::chronology::time_point{ common::chronology::from::string( value)};
                              else
                                 common::code::raise::error( common::code::casual::invalid_argument, "'", name, "' is not part of the valid set: ", deprecated::names());
                           }

                           casual::cli::pipe::forward::message( message);
                        };

                        auto handle_payload_message = [ handle_queue_message]( casual::cli::message::payload::Message& message)
                        {
                           casual::cli::message::queue::Message result;
                           result.correlation = message.correlation;
                           result.execution = message.execution;
                           result.payload = std::move( message.payload);

                           handle_queue_message( result);
                        };
                        
                     casual::cli::pipe::done::Scope done;

                        auto handler = casual::cli::message::dispatch::create(
                           casual::cli::pipe::forward::handle::defaults(),
                           std::move( handle_queue_message),
                           std::move( handle_payload_message),
                           std::ref( done)
                        );

                        // consume from casual-pipe
                        communication::stream::inbound::Device in{ std::cin};
                        common::message::dispatch::pump( 
                           casual::cli::pipe::condition::done( done), 
                           handler, in);

                        // done dtor will send Done downstream.
                     };

                     auto complete = []( bool help, auto values) -> std::vector< std::string>
                     { 
                        if( help) 
                           return { "<attribute-name>", "<value>"};

                        if( values.size() % 2 == 0)
                           return algorithm::container::create< std::vector< std::string>>( deprecated::names());   //{ "properties", "reply", "available"};
                        
                        if( ! values.empty() && range::back( values) == "available")
                           return { common::chronology::to::string( std::chrono::duration_cast< std::chrono::seconds>( platform::time::clock::type::now().time_since_epoch()))};

                        if( ! values.empty() && range::back( values) == "reply")
                           return local::queues();

                        return { "<value>"};
                     };

                     return argument::Option{
                        argument::option::one::many( std::move( invoke)),
                        std::move( complete),
                        { {}, { "--attributes"}},
                        R"(@deprecated use `attributes --properties/reply/available`)"
                     };

                  }

               } // deprecated

               auto option()
               {
                  struct Shared
                  {
                     std::optional< std::string> properties;
                     std::optional< std::string> reply;
                     std::optional< common::chronology::time_point> available;
                  };

                  auto shared = std::make_shared< Shared>();

                  auto properties = [ shared]()
                  {
                     auto invoke = [ shared]( std::string value)
                     {
                        shared->properties = std::move( value);
                        return argument::option::invoke::preemptive{};
                     };

                     return argument::Option{
                        std::move( invoke),
                        {{ "--properties"}},
                        "sets the 'properties' attribute on piped queue messages"
                     }( argument::cardinality::zero_one());
                  };


                  auto reply = [ shared]()
                  {
                     auto invoke = [ shared]( std::string value)
                     {
                        shared->reply = std::move( value);
                        return argument::option::invoke::preemptive{};
                     };

                     return argument::Option{
                        std::move( invoke),
                        complete::queues,
                        {{ "--reply"}},
                        "sets the 'reply' attribute on piped queue messages"
                     }( argument::cardinality::zero_one());
                  };

                  auto available = [ shared]()
                  {
                     auto invoke = [ shared]( std::string value)
                     {
                        shared->available = common::chronology::time_point{ common::chronology::from::string( value)};
                        return argument::option::invoke::preemptive{};
                     };

                     constexpr auto description = R"(sets the 'available' attribute on piped queue messages

value is absolute time since epoch ([+]?<value>[h|min|s|ms|us|ns])+
)";

                     return argument::Option{
                        std::move( invoke),
                        {{ "--available"}},
                        description
                     }( argument::cardinality::zero_one());
                  };

                  auto invoke = [ shared]()
                  {
                     auto handle_queue_message = [ &shared]( casual::cli::message::queue::Message& message)
                     {
                        if( shared->properties)
                           message.attributes.properties = *shared->properties;
                        if( shared->reply)
                           message.attributes.reply = *shared->reply;
                        if( shared->available)
                           message.attributes.available = *shared->available;
                        
                        casual::cli::pipe::forward::message( message);
                     };

                     auto handle_payload_message = [ handle_queue_message]( casual::cli::message::payload::Message& message)
                     {
                        casual::cli::message::queue::Message result;
                        result.correlation = message.correlation;
                        result.execution = message.execution;
                        result.payload = std::move( message.payload);

                        handle_queue_message( result);
                     };
                     
                     casual::cli::pipe::done::Scope done;

                     auto handler = casual::cli::message::dispatch::create(
                        casual::cli::pipe::forward::handle::defaults(),
                        std::move( handle_queue_message),
                        std::move( handle_payload_message),
                        std::ref( done)
                     );

                     // consume from casual-pipe
                     communication::stream::inbound::Device in{ std::cin};
                     common::message::dispatch::pump( 
                        casual::cli::pipe::condition::done( done), 
                        handler, in);

                     // done dtor will send Done downstream.
                  };

                  constexpr std::string_view description = R"(adds or mutates queue message attributes on piped messages

@note: part of casual-pipe)";

                  constexpr std::string_view extended = R"(
Examples:
   
   casual queue --dequeue a \
      | casual queue attributes --reply a.reply \
      | casual queue --enqueue a

   casual transaction --begin \
      | casual queue --dequeue a \
      | casual queue attributes \
         --reply a.reply \
         --properties foo \
         --available 1625077800s \
      | casual queue --enqueue a \
      | casual transaction --commit
)";

                  return argument::Option{
                     std::move( invoke),
                     {{ "attributes"}},
                     { description, extended}
                  }(
                     {
                        properties(),
                        reply(),
                        available()
                     },
                     argument::cardinality::range( 1, 3)
                  );
                 
               }

            } // attributes

            namespace restore
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     format::affected( queue::restore::queue( queues));
                  };

                  constexpr auto description = R"(restores messages to queue from error queue)";

                  constexpr auto extended = R"(
Messages will be restored to the provided queues, from their corresponding 
error queues.

Example:
   casual queue --restore a b c

   Will restore messages from 'a.error', 'b.error' and 'c.error' to
   'a', 'b' and 'c' respectively.

)";

                  return argument::Option{
                     std::move( invoke),
                     complete::queues,
                     {{  "--restore"}},
                     { description, extended}
                  };
               }
            } // restore

            namespace messages
            {
               namespace remove
               {
                  auto option()
                  {
                     struct Shared
                     {
                        bool force = false;
                     };

                     auto shared = std::make_shared< Shared>();

                     auto invoke = [ shared]( const std::string& queue, Uuid id, std::vector< Uuid> ids)
                     {
                        ids.insert( std::begin( ids), std::move( id));
                        auto removed = queue::messages::remove( queue, ids, shared->force);

                        algorithm::for_each( removed, []( auto& id)
                        {
                           std::cout << id << '\n';
                        });
                     };

                     auto complete = []( bool help, auto values) -> std::vector< std::string>
                     { 
                        if( help) 
                           return { "<queue>", "<id>"};
                        
                        if( values.empty())
                           return local::queues();
                        
                        return { "<value>"};
                     };

                     auto flag = argument::Option{ [ shared]()
                        {
                           shared->force = true;
                           return argument::option::invoke::preemptive{};
                        }, 
                        {{ "--force"}}, "force removal of message regardless of state"};

                     constexpr auto description = R"(removes specific messages from a given queue)";

                     return argument::Option{
                        std::move( invoke),
                        complete,
                        {{  "--remove-messages"}},
                        description}( { std::move( flag)});
                  }
               } // remove

               namespace recovery
               {
                  using Directive = ipc::message::group::message::recovery::Directive;

                  namespace detail
                  {
                     auto create_option( Directive directive, argument::option::Names names, std::string description)
                     {
                        auto invoke = [ directive]( std::vector< common::transaction::global::ID> gtrids)
                        {
                           for( auto& gtrid : call::recover( std::move( gtrids), directive))
                              common::log::line( std::cout, gtrid);
                        };

                        auto complete = []( bool help, auto values) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<gtrid>"};

                           return { "<value>"};
                        };

                        return argument::Option{
                           argument::option::one::many( std::move( invoke)),
                           complete,
                           std::move( names),
                           std::move( description)
                        };


                     }
                     
                  } // detail

                  auto option()
                  {
                     return argument::Option{
                        [](){},
                        {{ "--recover-transactions"}},
                        "recover global transactions with --commit or --rollback sub option"
                     }({
                        detail::create_option( Directive::commit, { { "--commit"}, {}}, "recover global transactions with commit"),
                        detail::create_option( Directive::rollback, { { "--rollback"}, {}}, "recover global transactions with rollback"),
                     }, argument::cardinality::one());
                  }

               } // recovery
            } // messages

            namespace clear
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     format::affected( queue::clear::queue( queues));
                  };

                  constexpr auto description = R"(clears all messages from provided queues)";

                  constexpr auto extended = R"(
Example:
   casual queue --clear a b c
)";

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     complete::queues,
                     {{  "--clear"}},
                     { description, extended}
                  };
               }

            } // clear

            namespace metric::reset
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     casual::service::protocol::binary::Call call;
                     call << CASUAL_NAMED_VALUE( queues);
                     auto reply = call( manager::admin::service::name::metric::reset);
                  };

                  constexpr auto description = R"(resets metrics for the provided queues)";

                  constexpr auto extended = R"(
if no queues are provided, metrics for all queues are reset.

Example:

   casual queue --metric-reset a b
)";

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     complete::queues,
                     {{ "-mr", "--metric-reset"}},
                     { description, extended}
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
                     terminal::format::pair::print( information::call());
                  };

                  return argument::Option{
                     std::move( invoke),
                     {{  "--information"}},
                     "collect aggregated information about queues in this domain"};
                  }

            } // information

            namespace deprecated
            {
               auto list_remote_queues()
               {
                  auto invoke = []()
                  {
                     auto state = call::state();
                     format::remote::queues( algorithm::sort( state.remote.queues));
                  };
                  
                  return argument::Option{
                     std::move( invoke),
                     argument::option::Names( {}, { "-r", "--list-remote"}),
                     R"(deprecated - use --list-instances)"
                  };
               }

               auto recover_transactions_commit()
               {
                  return argument::Option{
                     messages::recovery::detail::create_option( messages::recovery::Directive::commit, { {}, { "--recover-transactions-commit"}}, "use --recover-transactions --commit instead")
                  };
               }

               auto recover_transactions_rollback()
               {
                  return argument::Option{
                     messages::recovery::detail::create_option( messages::recovery::Directive::rollback, { {}, { "--recover-transactions-rollback"}}, "use --recover-transactions --rollback instead")
                  };
               }

            } // deprecated

         } // <unnamed>
      } // local

      argument::Option options()
      {
         return argument::Option{ [](){}, {{ "queue"}}, "queue related administration"}( {
            local::list::queues::option(),
            local::list::zombies::option(),
            local::list::queue::instances::option(),
            local::list::groups::option(),
            local::list::messages::option(),
            local::forward::option(),
            local::fanout::option(),
            local::restore::option(),
            local::enqueue::option(),
            local::dequeue::option(),
            local::peek::option(),
            local::consume::option(),
            local::attributes::option(),
            local::clear::option(),
            local::messages::remove::option(),
            local::messages::recovery::option(),
            local::metric::reset::option(),
            local::information::option(),
            casual::cli::state::option( &local::call::state),

            local::attributes::deprecated::option(),
            local::deprecated::list_remote_queues(),
            local::deprecated::recover_transactions_commit(),
            local::deprecated::recover_transactions_rollback(),
            local::forward::list::services::deprecated_option(),
            local::forward::list::queues::deprecated_option(),
            local::forward::list::groups::deprecated_option(),
            local::forward::scale::aliases::deprecated_option(),
         });
      }

      std::vector< std::tuple< std::string, std::string>> information()
      {
         return local::information::call();
      }
            

   } // queue::manager::admin::cli
} // casual



