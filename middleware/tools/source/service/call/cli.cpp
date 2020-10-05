//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "tools/service/call/cli.h"

#include "common/argument.h"
#include "common/optional.h"
#include "common/service/call/context.h"
#include "common/exception/handle.h"
#include "common/transaction/context.h"
#include "common/execute.h"
#include "common/terminal.h"
#include "common/message/dispatch.h"
#include "common/communication/select.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"

#include "casual/cli/pipe.h"

#include "domain/pending/message/send.h"
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

   namespace tools 
   {
      namespace service
      {
         namespace call
         {
            namespace local
            {
               namespace
               {
                  struct State
                  {
                     enum class Flag : short
                     {
                        done = 0,
                        pipe = 1,
                        ipc = 2,
                        multiplexing = pipe | ipc,
                     };

                     friend std::ostream& operator << ( std::ostream& out, Flag flag)
                     {
                        switch( flag)
                        {
                           case Flag::done: return out << "done";
                           case Flag::pipe: return out << "pipe";
                           case Flag::ipc: return out << "ipc";
                           case Flag::multiplexing: return out << "multiplexing";
                        }
                        return out << "<unknown>";
                     }


                     struct Lookup
                     {
                        casual::cli::message::Payload payload;
                        platform::size::type count{};

                        inline friend bool operator == ( const Lookup& lhs, const Uuid& rhs) { return lhs.payload.correlation == rhs;}

                        CASUAL_LOG_SERIALIZE(
                           CASUAL_SERIALIZE( payload);
                           CASUAL_SERIALIZE( count);
                        )
                     };

                     Flags< Flag> machine = Flag::pipe;



                     struct
                     {
                        std::vector< Uuid> calls;
                        std::vector< Lookup> lookups;

                        auto empty() const { return calls.empty() && lookups.empty();}

                        CASUAL_LOG_SERIALIZE(
                           CASUAL_SERIALIZE( calls);
                           CASUAL_SERIALIZE( lookups);
                        )

                     } pending;

                     std::string service;
                     platform::size::type iterations = 1;

                     CASUAL_LOG_SERIALIZE(
                        CASUAL_SERIALIZE( machine);
                        CASUAL_SERIALIZE( pending);
                        CASUAL_SERIALIZE( service);
                        CASUAL_SERIALIZE( iterations);
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
                     auto pipe( State& state)
                     {
                        auto handle_lookup = [&state]( casual::cli::message::Payload& message)
                        {
                           Trace trace{ "tools::service::call::local::handle::pipe::handle_lookup"};
                           //log::line( verbose::log, "state: ", state);

                           if( state.iterations <= 0)
                              return;

                           // send request(s)
                           {
                              common::message::service::lookup::Request request{ process::handle()};
                              request.correlation = message.correlation;
                              request.requested = state.service;
                              // we will wait 'for ever'.
                              request.context = decltype( request.context)::no_busy_intermediate;

                              algorithm::for_n( state.iterations, [&request]()
                              {
                                 Trace trace{ "tools::service::call::local::handle::pipe::handle_lookup eventually send"};
                                 casual::domain::pending::message::eventually::send( 
                                    communication::instance::outbound::service::manager::device(), 
                                    request);
                              });                              
                           }

                           state.pending.lookups.push_back( State::Lookup{ std::move( message), state.iterations});
                           // add ipc to the state machine (might be set already)
                           state.machine |= State::Flag::ipc;

                           // TODO sequential calls...

                           log::line( verbose::log, "state: ", state);

                        };

                        return common::message::dispatch::handler( communication::stream::inbound::Device{ std::cin},
                           casual::cli::pipe::forward::handle::defaults(),
                           std::move( handle_lookup),
                           [&state]( const casual::cli::message::pipe::Done&)
                           {
                              // remove the pipe from the state-machine
                              state.machine -= State::Flag::pipe;
                           }
                        );
                     }

                     auto ipc( State& state)
                     {
                        auto lookup_reply = [&state]( common::message::service::lookup::Reply& message)
                        {
                           Trace trace{ "tools::service::call::local::handle::ipc::lookup_reply"};
                           log::line( verbose::log, "message: ", message);

                           auto found = algorithm::find( state.pending.lookups, message.correlation);
                           assert( found);

                           auto& lookup = *found;

                           // call
                           {
                              common::message::service::call::caller::Request request{ common::buffer::payload::Send{ lookup.payload.payload}};
                              request.process = process::handle();
                              request.service = std::move( message.service);
                              request.pending = message.pending;

                              using Type = common::service::transaction::Type;
                              if( algorithm::compare::any( message.service.transaction, Type::automatic, Type::join, Type::branch))
                                 request.trid = lookup.payload.transaction.trid;
                              
                              // we let 'communication' create a new correlation id
                              state.pending.calls.push_back( casual::domain::pending::message::eventually::send( message.process, request));
                           }

                           if( --lookup.count == 0)
                              state.pending.lookups.erase( std::begin( found));

                           log::line( verbose::log, "state: ", state);
                        };

                        auto call_reply = [&state]( common::message::service::call::Reply& message)
                        {
                           Trace trace{ "tools::service::call::local::handle::ipc::call_reply"};
                           log::line( verbose::log, "message: ", message);

                           algorithm::trim( state.pending.calls, algorithm::remove( state.pending.calls, message.correlation));

                           casual::cli::message::Payload result{ process::handle()};
                           result.payload = std::move( message.buffer);
                           result.transaction.trid = message.transaction.trid;
                           result.transaction.code = message.transaction.state == decltype( message.transaction.state)::active ? code::tx::ok : code::tx::rollback;

                           casual::cli::pipe::forward::message( result);

                           // if we got no pending, we remove our self
                           if( state.pending.empty())
                              state.machine -= State::Flag::ipc;

                           log::line( verbose::log, "state: ", state);

                        };


                        return common::message::dispatch::handler( communication::ipc::inbound::device(),
                           std::move( lookup_reply),
                           std::move( call_reply)
                        );
                     }

                  } // handle
               
                  void call( State& state)
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
                           auto create_select_handler = [&directive]( auto fd, auto&& handler, auto& device)
                           {
                              directive.read.add( fd);
                              return communication::select::dispatch::create::reader( fd, [handler = std::move( handler), &device]( auto fd)
                              {
                                 handler( communication::device::blocking::next( device));
                              });
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

                     // we're done
                     casual::cli::pipe::done();
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

                  namespace option
                  {
                     auto examples( State& state)
                     {
                        auto invoke = [&state]()
                        {
                           // make sure we don't do any calls
                           state.machine = State::Flag::done;

                           std::cout << R"(examples:

json buffer:
   `host# echo '{ "key" : "some", "value": "json"}' | casual buffer --compose ".json/" | casual call --service a | casual buffer --extract`

fielded buffer:
   `host# cat some-fields.yaml | casual buffer --field-from-human yaml | casual call --service a | casual buffer --field-to-human json`

   where the format of _human-fields_ are (in this case yaml):
   
   fields:
     - name: "CUSTOMER_ID"
       value: "9999999"
     - name: "CUSTOMER_NAME"
       value: "foo bar"
   
from a dequeue
   `host# casual queue --dequeue qA | casual call --service a | casual queue --enqueue qB`
)";

                        };

                        return argument::Option{ invoke, { "--examples"}, "prints several examples of how casual call can be used"};

                     }
                  } // option
               } // <unnamed>
            } // local
         
            struct cli::Implementation
            {
               argument::Group options()
               {
                  auto invoked = [&]()
                  {
                     local::call( m_state);
                  };

                  constexpr auto transaction_information = R"([removed] use `casual transaction --begin` instead)";
                  constexpr auto asynchronous_information = R"([removed] use `casual --block true|false call ...` instead)";

                  auto deprecated = []( auto message)
                  {
                     return [message]( bool on)
                     {
                        if( on)
                           std::cerr << message << '\n';
                     };
                  };

                  return argument::Group{ invoked, [](){}, { "call"}, R"(generic service call

Reads buffer(s) from stdin and call the provided service, prints the reply buffer(s) to stdout.
Assumes that the input buffer to be in a conformant format, ie, created by casual.
Errors will be printed to stderr

@note: part of casual-pipe
)",
                     argument::Option( std::tie( m_state.service), local::complete::service, { "-s", "--service"}, "service to call")( argument::cardinality::one{}),
                     argument::Option( std::tie( m_state.iterations), { "--iterations"}, "number of iterations (default: 1) - this could be helpful for testing load"),
                     local::option::examples( m_state),

                     // deprecated
                     argument::Option( deprecated( asynchronous_information), argument::option::keys( {}, { "--asynchronous"}), asynchronous_information),
                     argument::Option( deprecated( transaction_information), argument::option::keys( {}, { "--transaction"}), transaction_information),
                  };
               }

               local::State m_state;
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            argument::Group cli::options() &
            {
               return m_implementation->options();
            }
         } // call
      } // manager  
   } // gateway
} // casual



