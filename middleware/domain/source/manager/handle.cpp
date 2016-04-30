//!
//! casual 
//!

#include "domain/manager/handle.h"

#include "domain/common.h"



#include "common/message/handle.h"


namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {

         namespace ipc
         {
            const communication::ipc::Helper& device()
            {
               static communication::ipc::Helper singleton{ communication::error::handler::callback::on::Terminate{ &handle::process::termination::event}};
               return singleton;
            }
         } // ipc

         namespace handle
         {



            namespace adjust
            {
               void instances( State& state)
               {
                  Trace trace{ "domain::manager::handle::adjust::instances"};

               }


            } // adjust

            namespace mandatory
            {
               void boot( State& state)
               {
                  Trace trace{ "domain::manager::handle::mandatory::boot"};

                  state.mandatory.emplace_back( common::process::spawn( "${CASUAL_HOME}/bin/casual-broker", {}));
                  state.mandatory.emplace_back( common::process::spawn( "${CASUAL_HOME}/bin/casual-transaction-manager", {}));
               }

            } // mandatory

            void boot( State& state)
            {
               Trace trace{ "domain::manager::handle::boot"};


            }


            void shutdown( State& state)
            {
               Trace trace{ "domain::manager::handle::shutdown"};

               //
               // We only want child and alarm signals
               //
               signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::child, signal::Type::alarm})};

               auto handle = handler( state);

               auto shutdown = state.shutdownorder();

               //
               // Configure instances to 0
               //
               range::for_each( shutdown, []( state::Batch& b){
                  range::for_each( b.executables, []( state::Executable& e){
                     e.configured_instances = 0;
                  });
               });

               range::for_each( shutdown, [&]( state::Batch& batch){
                  try
                  {
                     signal::timer::Scoped timer{ batch.timeout()};

                     log::information << "shutdown group " << batch.group.get().name << '\n';

                     range::for_each( batch.executables, scale::Executable{ state});

                     while( ! batch.offline())
                     {
                        handle( ipc::device().blocking_next());
                     }
                  }
                  catch( const exception::signal::Timeout&)
                  {
                     log::error << "failed to shutdown batch in a timely manner - action: keep going... - batch: " << batch << '\n';
                     //timeouts.push_back( std::move( batch));
                  }
               });
            }


            Base::Base( State& state) : m_state{ state} {}

            State& Base::state()
            {
               return m_state.get();
            }

            const State& Base::state() const
            {
               return m_state.get();
            }


            namespace scale
            {
               namespace
               {
                  void out( state::Executable& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::out"};
                     try
                     {
                        auto current = executable.instances.size();

                        while( current++ < executable.configured_instances)
                        {
                           auto pid = common::process::spawn( executable.path, executable.arguments, executable.environment.variables);
                           executable.instances.push_back( pid);
                        }
                     }
                     catch( const exception::invalid::Argument& e)
                     {
                        log::error << "failed to spawn executable: " << executable << " - " << e << '\n';
                     }
                  }

                  void in( const State& state, const state::Executable& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::in"};

                     assert( executable.configured_instances <= executable.instances.size());

                     //
                     // We only want child signals
                     //
                     signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::child})};

                     auto pids = range::make_reverse(
                           range::make(
                                 std::begin( executable.instances) + executable.configured_instances,
                                 std::end( executable.instances)));

                     std::vector< platform::pid::type> terminate;

                     range::for_each( pids, [&state, &terminate]( platform::pid::type pid){

                        auto found = range::find( state.processes, pid);

                        if( found)
                        {
                           try
                           {
                              ipc::device().non_blocking_send( found->second.queue, message::shutdown::Request{});
                           }
                           catch( const exception::queue::Unavailable&)
                           {
                              terminate.push_back( pid);
                           }
                        }
                        else
                        {
                           terminate.push_back( pid);
                        }
                     });

                     common::process::terminate( terminate);
                  }
               } //

               void Executable::operator () ( state::Executable& executable) const
               {
                  Trace trace{ "domain::manager::handle::scale::executable"};

                  if( executable.instances.size() < executable.configured_instances)
                  {
                     scale::out( executable);
                  }
                  else if( executable.instances.size() > executable.configured_instances)
                  {
                     scale::in( state(), executable);
                  }
               }

            } // scale


            namespace process
            {
               namespace termination
               {
                  void event( const common::process::lifetime::Exit& exit)
                  {
                     //
                     // We put a dead process event on our own ipc device, that
                     // will be handled later on.
                     //
                     common::message::process::termination::Event event{ exit};
                     communication::ipc::inbound::device().push( std::move( event));
                  }

                  void Registration::operator () ( const common::message::process::termination::Registration& message)
                  {
                     Trace trace{ "domain::manager::handle::process::termination::Registration"};

                     auto& listeners = state().termination.listeners;

                     auto found = range::find_if( listeners, common::process::Handle::equal::pid{ message.process.pid});
                     if( found)
                     {
                        *found = message.process;
                     }
                     else
                     {
                        listeners.push_back( message.process);
                     }

                     log << "termination.listeners: " << range::make( listeners);

                  }

                  void Event::operator () ( common::message::process::termination::Event& message)
                  {
                     Trace trace{ "domain::manager::handle::process::termination::Event"};

                     if( message.death.deceased())
                     {
                        //
                        // We don't want to handle any signals in this task
                        //
                        signal::thread::scope::Block block;

                        if( message.death.reason == common::process::lifetime::Exit::Reason::exited)
                        {
                           log::information << "process exited: " << message.death << '\n';
                        }
                        else
                        {
                           log::error << "process terminated: " << message.death << '\n';
                        }


                        state().remove_process( message.death.pid);

                        auto& listeners = state().termination.listeners;


                        //
                        // We send event to all listeners
                        //
                        {
                           common::message::pending::Message pending{ message,
                              range::transform( listeners, std::mem_fn( &common::process::Handle::queue))};

                           if( ! common::message::pending::send( pending,
                                 communication::ipc::policy::non::Blocking{}))
                           {
                              state().pending.replies.push_back( std::move( pending));
                           }
                        }
                     }
                  }
               } // termination

               namespace local
               {
                  namespace
                  {
                     struct Lookup : Base
                     {
                        using Base::Base;

                        bool operator () ( const common::message::process::lookup::Request& message)
                        {
                           auto reply = common::message::reverse::type( message);
                           reply.identification = message.identification;

                           auto send_reply = [&](){
                              if( ! ipc::device().non_blocking_send( message.process.queue, reply))
                              {
                                 state().pending.replies.emplace_back( reply, message.process.queue);
                              }
                           };

                           if( message.identification)
                           {
                              auto found = range::find( state().singeltons, message.identification);

                              if( found)
                              {
                                 reply.process = found->second;
                                 send_reply();
                              }
                              else if( message.directive == common::message::process::lookup::Request::Directive::direct)
                              {
                                 send_reply();
                              }
                              else
                              {
                                 return false;
                              }
                           }
                           else if( message.pid)
                           {
                              auto found = range::find( state().processes, message.pid);

                              if( found)
                              {
                                 reply.process = found->second;
                                 send_reply();
                              }
                              else if( message.directive == common::message::process::lookup::Request::Directive::direct)
                              {
                                 send_reply();
                              }
                              else
                              {
                                 return false;
                              }
                           }
                           else
                           {
                              // invalid
                              log::error << "invalid lookup request: " << '\n';
                           }
                           return false;
                        }
                     };

                  } // <unnamed>
               } // local

               void Connect::operator () ( common::message::inbound::ipc::Connect& message)
               {
                  Trace trace{ "domain::manager::handle::process::Connect"};

                  state().processes[ message.process.pid] = message.process;


                  auto& pending = state().pending.lookup;

                  range::trim( pending, range::remove_if( pending, local::Lookup{ state()}));

               }

               void Lookup::operator () ( const common::message::process::lookup::Request& message)
               {
                  Trace trace{ "domain::manager::handle::process::Lookup"};

                  if( ! local::Lookup{ state()}( message))
                  {
                     state().pending.lookup.push_back( message);
                  }
               }


            } // process

         } // handle

         common::message::dispatch::Handler handler( State& state)
         {
            Trace trace{ "domain::manager::handler"};

            return {
               common::message::handle::ping(),
               common::message::handle::Shutdown{},
               manager::handle::process::termination::Event{ state},
               manager::handle::process::termination::Registration{ state},
               manager::handle::process::Connect{ state},
               manager::handle::process::Lookup{ state},

            };

         }

      } // manager

   } // domain


} // casual
