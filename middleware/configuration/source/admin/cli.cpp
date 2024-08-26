//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/admin/cli.h"
#include "configuration/model/transform.h"
#include "configuration/model/load.h"

#include "common/argument.h"
#include "common/file.h"
#include "common/serialize/create.h"
#include "common/environment.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"
#include "common/result.h"
#include "common/terminal.h"
#include "common/event/listen.h"

#include "serviceframework/service/protocol/call.h"

#include "domain/manager/admin/server.h"

namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace admin
      {
         namespace local
         {
            namespace
            {
               auto format_list = []( auto, bool){ return std::vector< std::string>{ "json", "yaml", "xml", "ini"};};

               namespace event
               {
                  auto handler( std::vector< common::strong::correlation::id>& tasks) 
                  {
                     return message::dispatch::handler( communication::ipc::inbound::device(),
                        []( const message::event::process::Spawn& event)
                        {
                           message::event::terminal::print( std::cout, event);
                        },
                        []( const message::event::process::Exit& event)
                        {
                           message::event::terminal::print( std::cout, event);
                        },
                        [&tasks]( message::event::Task& event)
                        {
                           message::event::terminal::print( std::cout, event);
                           if( event.done())
                              if( algorithm::find( tasks, event.correlation))
                                 tasks.clear();
                        },
                        []( const message::event::sub::Task& event)
                        {
                           message::event::terminal::print( std::cout, event);
                        },
                        []( const message::event::Error& event)
                        {
                           message::event::terminal::print( std::cerr, event);
                        },
                        []( const message::event::Notification& event)
                        {
                           message::event::terminal::print( std::cout, event);
                        }
                     );
                  };

                  // generalization of the event handling
                  template< typename I, typename... Args>
                  void invoke( I&& invocable, Args&&... arguments)
                  {
                     // if no-block we don't mess with events
                     if( ! terminal::output::directive().block())
                     {
                        invocable( std::forward< Args>( arguments)...);
                        return;
                     }

                     decltype( invocable( std::forward< Args>( arguments)...)) tasks;

                     auto condition = common::event::condition::compose(
                        common::event::condition::prelude( [&]()
                        {
                           tasks = invocable( std::forward< Args>( arguments)...);
                        }),
                        common::event::condition::done( [&tasks]()
                        { 
                           return tasks.empty();
                        })
                     );

                     // listen for events
                     common::event::listen( 
                        condition,
                        local::event::handler( tasks));
                  }
                  

               } // event

               namespace call
               {
                  auto get()
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( casual::domain::manager::admin::service::name::configuration::get).extract< casual::configuration::user::Model>();
                  }

                  auto post( const casual::configuration::user::Model& model)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( casual::domain::manager::admin::service::name::configuration::post, model).extract< std::vector< common::strong::correlation::id>>();
                  }

                  auto put( const casual::configuration::user::Model& model)
                  {
                     serviceframework::service::protocol::binary::Call call;
                     return call( casual::domain::manager::admin::service::name::configuration::put, model).extract< std::vector< common::strong::correlation::id>>();
                  }
               
               } // call

               struct State
               {
                  std::string format{ "yaml"};
               };

               auto load( std::vector< std::string> globs)
               {
                  return model::load( file::find( std::move( globs)));
               }

               auto validate()
               {
                  auto invoke = []( std::vector< std::string> globs)
                  {
                     local::load( std::move( globs));
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     { "--validate"},
                     R"(validates configuration from supplied glob patterns

On success exit with 0, on error not 0, and message printed to stderr)"
                  };
               }

               auto normalize( State& state)
               {
                  auto invoke = [&state]( std::vector< std::string> globs)
                  {
                     // load the model, transform back to user model...
                     auto domain = model::transform( local::load( std::move( globs)));
                     auto writer = serialize::create::writer::from( state.format);
                     writer << domain;
                     writer.consume( std::cout);
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     { "--normalize"},
                     R"(normalizes the supplied configuration glob pattern to stdout

The format is default yaml, but could be supplied via the --format option)"
                  };
               }

               auto format( State& state)
               {
                  auto complete = []( auto values, auto help)
                  {
                     return std::vector< std::string>{ "yaml", "json", "ini", "xml"};
                  };

                  return argument::Option{
                     std::tie( state.format),
                     std::move( complete),
                     { "--format"},
                     R"(defines what format should be used)"
                  };
               }

               namespace set_operations
               {
                  namespace local
                  {
                     namespace
                     {
                        template< typename F>
                        void set_operation( std::vector< std::string> globs, State& state, F&& func)
                        {
                           casual::configuration::user::Model model;
                           auto archive = serialize::create::reader::consumed::from( state.format, std::cin);
                           archive >> model;

                           auto a = model::transform( model);
                           auto b = admin::local::load( std::move( globs));

                           a = func( a, std::move( b));

                           auto writer = serialize::create::writer::from( state.format);
                           writer << model::transform( a);
                           writer.consume( std::cout);
                        }
                     } // <unnamed>
                  } // local

                  auto set_union( State& state)
                  {
                     auto invoke = [ &state]( std::vector< std::string> globs)
                     {
                        local::set_operation( globs, state, []( auto lhs, auto rhs)
                        {
                           return set_union( lhs, std::move( rhs));
                        });
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--union"},
                        R"(union of configuration from stdin(lhs) and supplied glob pattern(rhs), outputs to stdout
   rhs has precedence over lhs
                        
   The format is default yaml, but could be supplied via the --format option)"
                     };
                  }

                  auto set_difference( State& state)
                  {
                     auto invoke = [ &state]( std::vector< std::string> globs)
                     {
                        local::set_operation( globs, state, []( auto lhs, auto rhs)
                        {
                           return set_difference( lhs, std::move( rhs));
                        });
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--difference"},
                        R"(difference of configuration from stdin(lhs) and supplied glob pattern(rhs), outputs to stdout
   lhs has precedence over rhs
                        
   The format is default yaml, but could be supplied via the --format option)"
                     };
                  }

                  auto set_intersection( State& state)
                  {
                     auto invoke = [ &state]( std::vector< std::string> globs)
                     {
                        local::set_operation( globs, state, []( auto lhs, auto rhs)
                        {
                           return set_intersection( lhs, std::move( rhs));
                        });
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--intersection"},
                        R"(intersection of configuration from stdin(lhs) and supplied glob pattern(rhs), outputs to stdout
   lhs has precedence over rhs
                        
   The format is default yaml, but could be supplied via the --format option)"
                     };
                  }
               } // set_operation


               namespace runtime
               {
                  namespace detail
                  {
                     auto get( auto option_name, auto description)
                     {
                        auto invoke = []( const std::optional< std::string>& format)
                        {
                           auto archive = common::serialize::create::writer::from( format.value_or( "yaml"));
                           archive << call::get();
                           archive.consume( std::cout);
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           format_list, 
                           option_name, 
                           description
                        };
                     }

                     auto post( auto option_name, auto description)
                     {
                        auto invoke = []( const std::string& format)
                        {
                           casual::configuration::user::Model model;
                           auto archive = common::serialize::create::reader::consumed::from( format, std::cin);
                           archive >> model;

                           event::invoke( call::post, model);
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           format_list, 
                           option_name, 
                           description
                        };  
                     }


                     auto edit( auto option_name, auto description)
                     {
                        auto invoke = []( const std::optional< std::string>& format)
                        {                        
                           auto get_editor_path = []() -> std::filesystem::path
                           {
                              return environment::variable::get( environment::variable::name::terminal::editor).value_or( 
                                 environment::variable::get( "VISUAL").value_or( 
                                    environment::variable::get( "EDITOR").value_or( "vi")));
                           };

                           auto current = call::get();

                           auto get_configuration_file = []( auto& domain, auto format)
                           {
                              file::output::Truncate file{ file::temporary( format)};
                              file::scoped::Path scoped{ file.path()};
                              
                              auto archive = common::serialize::create::writer::from( format);
                              archive << domain;
                              archive.consume( file);

                              return scoped;
                           };

                           auto start_editor = []( auto editor, const auto& file)
                           {
                              const auto command = string::compose( editor, ' ', file);
                              common::log::line( verbose::log, "command: ", command);

                              posix::result( ::system( command.data()), "::system( ", command, ')');
                           };

                           // sink child signal from _editor_
                           signal::callback::registration< code::signal::child>( [](){});

                           auto file = get_configuration_file( current, format.value_or( "yaml"));

                           start_editor( get_editor_path(), file);

                           auto wanted = casual::configuration::model::load( { file});

                           if( wanted == casual::configuration::model::transform( current))
                           {
                              common::log::line( std::clog, "no configuration changes detected");
                              return;
                           }

                           event::invoke( call::post, casual::configuration::model::transform( wanted));
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           format_list, 
                           option_name, 
                           description
                        };
                     }

                     auto put( auto option_name, auto description)
                     {
                        auto invoke = []( const std::string& format)
                        {
                           casual::configuration::user::Model model;
                           auto archive = common::serialize::create::reader::consumed::from( format, std::cin);
                           archive >> model;

                           event::invoke( call::put, model);
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           format_list, 
                           option_name, 
                           description
                        };
                     }
                  } // detail

                  auto get()
                  {
                     return detail::get( argument::option::keys( { "--get"}, {}), "get current configuration");
                  }

                  auto post()
                  {
                     constexpr auto description = R"(reads configuration from stdin and replaces the domain configuration

casual will try to conform to the new configuration as smooth as possible. Although, there could be some "noise"
depending on what parts are updated.
)";

                     return detail::post( argument::option::keys( { "--post"}, {}), description);  
                  }


                  auto edit()
                  {
                     constexpr auto description = R"(get current configuration, starts an editor, on quit the edited configuration is posted.

The editor is deduced from the following environment variables, in this order:
* CASUAL_TERMINAL_EDITOR
* VISUAL
* EDITOR

If none is set, `vi` is used.

If no changes are detected, no update will take place.
)";


                     return detail::edit( argument::option::keys( { "--edit"}, {}), description);  
                  }

                  auto put()
                  {
                     constexpr auto description = R"(reads configuration from stdin and adds or updates parts

The semantics are similar to http PUT:
* every key that is found is treated as an update of that _entity_
* every key that is NOT found is treated as a new _entity_ and added to the current state 
)";

                     return detail::put( argument::option::keys( { "--put"}, {}), description);     
                  }

                  namespace groups
                  {
                     auto completer = []( auto values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<name>.."};

                        auto model = call::get();

                        if( ! model.domain || ! model.domain->groups)
                           return { "<value>"};

                        
                        return algorithm::transform( *model.domain->groups, []( auto group){ return group.name;});
                     }; 
                  
                     
                     auto enable()
                     {
                        auto invoke = []( std::vector< std::string> names)
                        {
                           auto enable_group = [ names = std::move( names)]( auto& group)
                           {
                              if( algorithm::find( names, group.name))
                                 group.enabled = true;
                           };

                           auto model = call::get();

                           if( ! model.domain || ! model.domain->groups)
                              return;

                           algorithm::for_each( *model.domain->groups, enable_group);

                           event::invoke( call::post, model);
                        };

                        return argument::Option{ 
                           argument::option::one::many( std::move( invoke)), 
                           completer, 
                           { "--enable-groups"}, 
                           R"(INCUBATION enables groups
Enables groups that provided group names matches.

This effects entities that has memberships to enabled groups
(domain:Server, domain:Executable, queue::Forward)

@attention INCUBATION - might change during, or in between minor version.
)"
                        };
                     }

                     auto disable()
                     {
                        auto invoke = []( std::vector< std::string> names)
                        {
                           auto disable_group = [ names = std::move( names)]( auto& group)
                           {
                              if( algorithm::find( names, group.name))
                                 group.enabled = false;
                           };

                           auto model = call::get();

                           if( ! model.domain || ! model.domain->groups)
                              return;

                           algorithm::for_each( *model.domain->groups, disable_group);

                           event::invoke( call::post, model);
                        };

                        return argument::Option{ 
                           argument::option::one::many( std::move( invoke)), 
                           completer, 
                           { "--disable-groups"}, 
                           R"(INCUBATION disables groups
Disables groups that provided group names matches.

This effects entities that has memberships to enabled groups
(domain:Server, domain:Executable, queue::Forward)

@attention INCUBATION - might change during, or in between minor version.
)"
                        };
                     }
                     
                  } // groups

                  namespace disable
                  {
                     
                  } // disable


               } // runtime
            } // <unnamed>
         } // local

         namespace deprecated
         {
            common::argument::Option get() 
            { 
               return local::runtime::detail::get( argument::option::keys( {}, { "--configuration-get"}), "@deprecated: use `casual configuration --get`");
            }

            common::argument::Option post()
            { 
               return local::runtime::detail::post( argument::option::keys( {}, { "--configuration-post"}), "@deprecated: use `casual configuration --post`");
            }

            common::argument::Option put()
            { 
               return local::runtime::detail::put( argument::option::keys( {}, { "--configuration-put"}), "@deprecated: use `casual configuration --put`");
            }
            
            common::argument::Option edit()
            { 
               return local::runtime::detail::edit( argument::option::keys( {}, { "--configuration-edit"}), "@deprecated: use `casual configuration --edit`");
            }
         } // deprecated


         struct CLI::Implementation
         {
            
            auto options()
            {
               constexpr auto description = R"(configuration utility - does NOT actively configure anything
               
Used to check and normalize configuration
)";
               return argument::Group{ [](){}, { "configuration"}, description,
                  local::runtime::get(),
                  local::runtime::post(),
                  local::runtime::put(),
                  local::runtime::edit(),
                  local::runtime::groups::enable(),
                  local::runtime::groups::disable(),
                  local::normalize( state),
                  local::validate(),
                  local::format( state),
                  local::set_operations::set_union( state),
                  local::set_operations::set_difference( state),
                  local::set_operations::set_intersection( state)
               };
            }

            local::State state;
         };

         CLI::CLI() = default; 
         CLI::~CLI() = default; 

         common::argument::Group CLI::options() &
         {
            return m_implementation->options();
         }

      } // admin
      
   } // configuration
} // casual