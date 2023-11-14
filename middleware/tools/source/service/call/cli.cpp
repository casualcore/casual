//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "tools/service/call/cli.h"

#include "common/argument.h"
#include <optional>
#include "common/service/call/context.h"
#include "common/exception/capture.h"
#include "common/exception/format.h"
#include "common/transaction/context.h"
#include "common/execute.h"
#include "common/terminal.h"
#include "common/message/dispatch.h"
#include "common/communication/select.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"

#include "casual/cli/pipe.h"

#include "service/manager/admin/api.h"


#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <cstring>

#include "xatmi.h"

namespace casual 
{
   using namespace common;

   namespace tools::service::call
   {

      namespace local
      {
         namespace
         {
            namespace flush
            {
               template< typename D, typename M>
               auto send( D&& destination, M&& message)
               {
                  auto result = communication::device::non::blocking::send( destination, message);
                  while( ! result)
                  {
                     communication::ipc::inbound::device().flush();
                     result = communication::device::non::blocking::send( destination, message);
                  }
                  return result;
               }
            }

            struct Arguments
            {
               std::string service;
               platform::size::type iterations = 1;
               bool show_examples = false;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( iterations);
                  CASUAL_SERIALIZE( show_examples);
               )
            };

            struct State
            {
               State( Arguments arguments) : arguments{ std::move( arguments)} {}

               enum class Flag : short
               {
                  done = 0,
                  pipe = 1,
                  ipc = 2,
                  multiplexing = pipe | ipc,
               };
               
               constexpr friend auto description( Flag flag)
               {
                  switch( flag)
                  {
                     case Flag::done: return std::string_view{ "done"};
                     case Flag::pipe: return std::string_view{ "pipe"};
                     case Flag::ipc: return std::string_view{ "ipc"};
                     default: return std::string_view{ "<unknown>"};
                  }
               }

               Flags< Flag> machine = Flag::pipe;

               struct
               {
                  std::vector< strong::correlation::id> calls;

                  auto empty() const { return calls.empty();}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( calls);
                  )

               } pending;

               Arguments arguments;
               common::transaction::ID current;
               casual::cli::pipe::done::Scope done;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( machine);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( arguments);
                  CASUAL_SERIALIZE( current);
                  CASUAL_SERIALIZE( done);
               )
            };

            namespace service
            {
               auto state()
               {
                  auto result = casual::service::manager::admin::api::state();

                  return common::algorithm::transform( result.services, []( auto& s)
                  {
                     return std::move( s.name);
                  });
               }
            }

            namespace handle
            {
               namespace detail
               {
                  auto lookup( std::string service)
                  {
                     Trace trace{ "tools::service::call::local::handle::detail::lookup"};

                     common::message::service::lookup::Request request{ process::handle()};
                     request.requested = std::move( service);
                     request.context.semantic = decltype( request.context.semantic)::no_busy_intermediate;
                     auto correlation = local::flush::send( communication::instance::outbound::service::manager::device(), request);

                     return communication::ipc::receive< common::message::service::lookup::Reply>( correlation);
                  };

                  template< typename M>
                  auto call( State& state, M& message)
                  {
                     Trace trace{ "tools::service::call::local::handle::detail::call"};

                     auto lookup = detail::lookup( state.arguments.service);
                     log::line( verbose::log, "lookup: ", lookup);

                     if( lookup.state == decltype( lookup.state)::absent)
                        code::raise::error( code::xatmi::no_entry);

                     // call
                  
                     common::message::service::call::caller::Request request{ common::buffer::payload::Send{ message.payload}};
                     request.process = process::handle();
                     request.service = lookup.service;
                     request.pending = lookup.pending;
                     request.correlation = strong::correlation::id::emplace( uuid::make());
                     // set trid, if any.
                     request.trid = state.current;
                     
                     return local::flush::send( lookup.process.ipc, request);
                  
                  }

                  void reply( State& state, common::message::service::call::Reply& message)
                  {
                     Trace trace{ "tools::service::call::local::handle::detail::reply"};
                     log::line( verbose::log, "message: ", message);
                     
                     // only log error to stderr. 
                     // TODO: is this "enough"?.
                     if( message.code.result != decltype( message.code.result)::ok)
                     {
                        // set the pipe in error-state to let downstream know
                        state.done.state( decltype( state.done.state())::error);
                        exception::format::terminal( std::cerr, exception::compose( message.code.result));
                     }

                     casual::cli::message::payload::Message result;
                     result.correlation = message.correlation;
                     result.execution = message.execution;
                     result.payload = std::move( message.buffer);
                     result.code = message.code;

                     log::line( verbose::log, "result: ", result);

                     casual::cli::pipe::forward::message( result);
                  }

               } // detail

               auto pipe( State& state)
               {
                  auto handle_payload = [&state]( auto& message)
                  {
                     Trace trace{ "tools::service::call::local::handle::pipe"};
                     log::line( verbose::log, "state: ", state);

                     if( state.arguments.iterations <= 0)
                        return;

                     algorithm::for_n( state.arguments.iterations, [&]()
                     {
                        // we let 'communication' create a new correlation id
                        state.pending.calls.push_back( detail::call( state, message));

                        // add ipc to the state machine (might be set already)
                        state.machine |= State::Flag::ipc;
                     });

                     log::line( verbose::log, "state: ", state);
                  };

                  return casual::cli::message::dispatch::create(
                     casual::cli::pipe::forward::handle::defaults(),
                     // make sure to get the ongoing transaction, if any
                     casual::cli::pipe::transaction::handle::current( state.current),
                     casual::cli::pipe::handle::payloads( std::move( handle_payload)),
                     [ &state]( const casual::cli::message::pipe::Done& message)
                     {
                        state.done( message);
                        // remove the pipe from the state-machine
                        state.machine -= State::Flag::pipe;
                     }
                  );
               }

               auto ipc( State& state)
               {
                  return common::message::dispatch::handle::protocol::ipc::create(
                     [ &state]( common::message::service::call::Reply& message)
                     {
                        Trace trace{ "tools::service::call::local::handle::ipc"};

                        if( auto found = algorithm::find( state.pending.calls, message.correlation))
                           state.pending.calls.erase( std::begin( found));
                        else
                           log::line( log::category::error, "failed to correlate reply: ", message.correlation);

                        detail::reply( state, message);

                        // if we got no pending, we remove our self
                        if( state.pending.empty())
                           state.machine -= State::Flag::ipc;

                        log::line( verbose::log, "state: ", state);
                     }
                  );
               }
            } // handle

            namespace blocking
            {
               void call( State state)
               {
                  Trace trace{ "tools::service::call::local::blocking::call"};
                  log::line( verbose::log, "state: ", state);
                  log::line( verbose::log, "state.machine == State::Flag::done: ", state.machine == State::Flag::done);

                  communication::stream::inbound::Device pipe{ std::cin};

                  auto condition = []( State& state)
                  {
                     return common::message::dispatch::condition::compose( 
                        common::message::dispatch::condition::done( [&state]()
                        {  
                           return state.machine == State::Flag::done;
                        }));
                  };

                  auto handler = []( State& state)
                  {
                     auto handle_payload = [ &state]( auto& message)
                     {
                        Trace trace{ "tools::service::call::local::blocking::call handler"};
                        common::log::line( verbose::log, "message: ", message);

                        if( state.arguments.iterations <= 0)
                           return;

                        algorithm::for_n( state.arguments.iterations, [&]()
                        {
                           handle::detail::call( state, message);

                           common::message::service::call::Reply reply;

                           communication::device::blocking::receive( 
                              communication::ipc::inbound::device(),
                              reply);

                           handle::detail::reply( state, reply);
                        });
                     };

                     return casual::cli::message::dispatch::create(
                        casual::cli::pipe::forward::handle::defaults(),
                        casual::cli::pipe::transaction::handle::current( state.current),
                        casual::cli::pipe::handle::payloads( std::move( handle_payload)),
                        [ &state]( const casual::cli::message::pipe::Done& message)
                        {
                           common::log::line( verbose::log, "done: ", message);
                           state.done( message);
                           state.machine = State::Flag::done;
                        }
                     );
                  };

                  common::message::dispatch::pump( 
                     condition( state), 
                     handler( state), 
                     pipe);

                  // we're done, state.done dtor will pipe done downstream;
               }
            } // blocking
         
            void call( State state)
            {
               Trace trace{ "tools::service::call::local::call"};
               log::line( verbose::log, "state: ", state);

               communication::stream::inbound::Device pipe{ std::cin};
               auto& ipc = communication::ipc::inbound::device();

               // will be 'done' if the state machine differs from origin
               auto state_machine_condition = [&state]( auto origin)
               {
                  return common::message::dispatch::condition::compose( 
                     common::message::dispatch::condition::done( [origin,&state]()
                     {  
                        return state.machine != origin;
                     }));
               };

               // start the 'state machine'. The handlers, created above, will
               // change the 'state machine' as stuff happens. We check the 
               // the state changes in the 'condition-done' via state_machine_condition

               while( state.machine != State::Flag::done)
               {
                  log::line( verbose::log, "state.machine: ", state.machine);

                  if( state.machine == State::Flag::pipe)
                  {
                     // just listen to the pipe 
                     common::message::dispatch::pump( 
                        state_machine_condition( State::Flag::pipe), 
                        handle::pipe( state), 
                        pipe);
                  }
                  else if( state.machine == State::Flag::ipc)
                  {
                     // just listen to ipc 
                     common::message::dispatch::pump( 
                        state_machine_condition( State::Flag::ipc), 
                        handle::ipc( state), 
                        ipc);
                  }
                  else if( state.machine == State::Flag::multiplexing)
                  {
                     communication::select::Directive directive;
                     auto create_select_handler = [ &directive]( auto fd, auto handler, auto& device)
                     {
                        directive.read.add( fd);
                        return [fd, handler = std::move( handler), &device]( auto descriptor, communication::select::tag::read) mutable
                        {
                           if( fd != descriptor)
                              return false;
                              
                           handler( communication::device::blocking::next( device));
                           return true;
                        };
                     };

                     communication::select::dispatch::pump(
                        state_machine_condition( State::Flag::multiplexing),
                        directive,
                        // the ipc handler
                        create_select_handler( ipc.connector().descriptor(), handle::ipc( state), ipc),
                        // the pipe handler
                        create_select_handler( file::descriptor::standard::in(), handle::pipe( state), pipe)
                     );
                  }
               }

               // we're done, state.done dtor will pipe done downstream;
            }

            namespace complete
            {
               auto service = []( auto values, bool help) -> std::vector< std::string>
               {
                  if( help) 
                     return { "<service>"};

                  try 
                  {
                     return service::state();
                  }
                  catch( ...)
                  {
                     return { "<value>"};
                  }
               };
            } // complete


            auto show_examples()
            {
               std::cout << R"(examples:

json buffer:
`host# echo '{ "key" : "some", "value": "json"}' | casual buffer --compose ".json/" | casual call --service a | casual buffer --extract`

fielded buffer:
`host# cat some-fields.yaml | casual buffer --field-from-human yaml | casual call --service a | casual buffer --field-to-human json`

where the format of _human-fields_ are:

```yaml
fields:
  - name: "CUSTOMER_ID"
    value: "9999999"
  - name: "CUSTOMER_NAME"
    value: "foo bar"
```

from a dequeue
`host# casual queue --dequeue qA | casual call --service a | casual queue --enqueue qB`
)";

            }
   
         } // <unnamed>
      } // local
   
      struct cli::Implementation
      {
         argument::Group options()
         {
            auto invoked = [&]()
            {
               if( m_arguments.show_examples)
               {
                  local::show_examples();
                  return;
               }

               if( terminal::output::directive().block())
                  local::blocking::call( std::move( m_arguments));
               else
                  local::call( std::move( m_arguments));
            };

            constexpr auto transaction_information = R"([removed] use `casual transaction --begin` instead)";
            constexpr auto asynchronous_information = R"([removed] use `casual --block true|false call ...` instead)";

            auto deprecated = []( auto message)
            {
               return [message]( bool) { std::cerr << message << '\n';};
            };

            return argument::Group{ invoked, [](){}, { "call"}, R"(generic service call

Reads buffer(s) from stdin and call the provided service, prints the reply buffer(s) to stdout.
Assumes that the input buffer to be in a conformant format, ie, created by casual.
Errors will be printed to stderr

@note: part of casual-pipe
)",
               argument::Option( std::tie( m_arguments.service), local::complete::service, { "-s", "--service"}, "service to call")( argument::cardinality::one{}),
               argument::Option( std::tie( m_arguments.iterations), { "--iterations"}, "number of iterations (default: 1) - this could be helpful for testing load"),
               argument::Option{ argument::option::toggle( m_arguments.show_examples), { "--examples"}, "prints several examples of how casual call can be used"},


               // deprecated
               argument::Option( deprecated( asynchronous_information), argument::option::keys( {}, { "--asynchronous"}), asynchronous_information),
               argument::Option( deprecated( transaction_information), argument::option::keys( {}, { "--transaction"}), transaction_information),
            };
         }

         local::Arguments m_arguments;
      };

      cli::cli() = default; 
      cli::~cli() = default; 

      argument::Group cli::options() &
      {
         return m_implementation->options();
      }

   } // tools::service::call
} // casual



