//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/manager/admin/cli.h"


#include "serviceframework/namevaluepair.h"
#include "serviceframework/archive/create.h"
#include "serviceframework/service/protocol/call.h"

#include "service/manager/admin/managervo.h"
#include "service/manager/admin/server.h"
#include "service/manager/admin/api.h"

#include "domain/manager/admin/vo.h"
#include "domain/manager/admin/server.h"


#include "common/file.h"
#include "common/argument.h"
#include "common/chronology.h"
#include "common/terminal.h"
#include "common/server/service.h"
#include "common/exception/handle.h"




//
// std
//
#include <iostream>
#include <iomanip>
#include <limits>




namespace casual
{

   using namespace common;

   namespace service
   {

      namespace manager
      {


         namespace global
         {
            bool admin_services = false;
         } // global



         namespace call
         {
            namespace metric
            {

               std::vector< std::string> reset( const std::vector< std::string>& services)
               {
                  serviceframework::service::protocol::binary::Call call;
                  call << CASUAL_MAKE_NVP( services);

                  auto reply = call( admin::service::name::metric::reset());

                  std::vector< std::string> result;

                  reply >> CASUAL_MAKE_NVP( result);

                  return result;
               }

            } // metric

            struct State
            {
               admin::StateVO service;
               casual::domain::manager::admin::vo::State domain;
            };

            State instances()
            {
               State state;

               serviceframework::service::protocol::binary::Call call;
               auto reply = call( casual::domain::manager::admin::service::name::state());

               reply >> CASUAL_MAKE_NVP( state.domain);

               state.service = admin::api::state();

               return state;
            }



         } // call

         namespace normalized
         {
            struct Instance : admin::instance::Base
            {
               enum class State : char
               {
                  idle,
                  busy,
                  remote,
               };

               Instance( const admin::instance::LocalVO& local) : admin::instance::Base{ local}, state{ static_cast< State>( local.state)} {}
               Instance( const admin::instance::RemoteVO& remote) : admin::instance::Base{ remote}, state{ State::remote} {}

               State state;
            };


            std::vector< Instance> instances( const admin::StateVO& state)
            {
               std::vector< Instance> result;

               algorithm::copy( state.instances.local, std::back_inserter( result));
               algorithm::copy( state.instances.remote, std::back_inserter( result));

               return result;
            }

            namespace service
            {

               struct Instance
               {
                  enum class State : char
                  {
                     idle,
                     busy,
                     remote,
                     exiting,
                  };

                  Instance( const admin::ServiceVO& service) : service{ service} {}

                  common::process::Handle process;

                  std::size_t hops = 0;


                  std::reference_wrapper< const admin::ServiceVO> service;

                  const casual::domain::manager::admin::vo::Executable* executable = nullptr;

                  State state = State::remote;
               };

               namespace local
               {
                  namespace
                  {
                     namespace lookup
                     {
                        const casual::domain::manager::admin::vo::Executable* executable( const call::State& state, strong::process::id pid)
                        {
                           auto found = algorithm::find_if( state.domain.executables, [=]( const casual::domain::manager::admin::vo::Executable& e){
                              return algorithm::find( e.instances, pid);
                           });

                           if( found)
                           {
                              return &(*found);
                           }
                           return nullptr;
                        }

                     } // lookup
                  } // <unnamed>
               } // local

               std::vector< Instance> instances( const call::State& state)
               {
                  std::vector< Instance> result;

                  for( auto& service : state.service.services)
                  {
                     for( auto& i : service.instances.local)
                     {
                        auto local = algorithm::find( state.service.instances.local, i.pid);
                        Instance instance{ service};
                        instance.state = local->state == admin::instance::LocalVO::State::idle ? Instance::State::idle : Instance::State::busy;
                        instance.process.pid = i.pid;
                        instance.executable = local::lookup::executable( state, i.pid);
                        result.push_back( std::move( instance));
                     }

                     for( auto& i : service.instances.remote)
                     {
                        Instance instance{ service};
                        instance.process.pid = i.pid;
                        instance.hops = i.hops;
                        instance.executable = local::lookup::executable( state, i.pid);
                        result.push_back( std::move( instance));
                     }
                  }

                  return result;
               }

            } // service

         } // normalized


         namespace format
         {
            struct state_base : std::reference_wrapper< const admin::StateVO>
            {
               using std::reference_wrapper< const admin::StateVO>::reference_wrapper;
            };



            namespace instance
            {
               struct base_instances
               {
                  using instances_type = std::vector< normalized::Instance>;
                  using range_type = range::type_t< instances_type>;

                  base_instances( instances_type& instances) : m_instances( instances) {}


                  template< typename T>
                  range_type instances( const std::vector< T>& instances) const
                  {
                     //
                     // We want to find the intersection between the argument instances and the
                     // total instances we hold
                     //

                     return std::get< 0>( algorithm::intersection( m_instances, instances, []( const normalized::Instance& ni, const T& i){
                        return ni.process.pid == i.pid;
                     }));
                  }


                  instances_type& m_instances;
               };


               namespace local
               {
                  struct total
                  {
                     std::size_t operator () ( const admin::ServiceVO& value) const
                     {
                        return value.instances.local.size();
                     }
                  };


                  struct busy : base_instances
                  {
                     using base_instances::base_instances;

                     std::size_t operator () ( const admin::ServiceVO& value) const
                     {
                        auto instances = base_instances::instances( value.instances.local);

                        return algorithm::count_if( instances, []( const normalized::Instance& i){
                           return i.state == normalized::Instance::State::busy;
                        });
                     }
                  };

                  struct pending_base : state_base
                  {
                     using state_base::state_base;

                     std::size_t pending( const std::string& service) const
                     {
                        return algorithm::count_if( get().pending, [&]( const admin::PendingVO& p){
                           return p.requested == service;
                        });
                     }
                  };

                  struct pending : pending_base
                  {
                     using pending_base::pending_base;

                     std::size_t operator () ( const admin::ServiceVO& value) const
                     {
                        return pending_base::pending( value.name);
                     }
                  };


               } // local

               namespace remote
               {
                  struct total
                  {
                     std::size_t operator () ( const admin::ServiceVO& value) const
                     {
                        return value.instances.remote.size();
                     }
                  };


               } // remote



            } // instances


            struct format_instances
            {
               template< typename T>
               std::size_t operator () ( const T& value) const { return value.instances.size();}
            };

            auto services( const admin::StateVO& state)
            {

               static auto instances = normalized::instances( state);

               struct format_timeout
               {
                  double operator () ( const admin::ServiceVO& value) const
                  {
                     using second_t = std::chrono::duration< double>;
                     return std::chrono::duration_cast< second_t>( value.timeout).count();
                  }
               };


               struct format_mode
               {
                  const char* operator () ( const admin::ServiceVO& value) const
                  {
                     static std::map< common::service::transaction::Type, const char*> mapping{
                        { common::service::transaction::Type::automatic, "auto"},
                        { common::service::transaction::Type::join, "join"},
                        { common::service::transaction::Type::atomic, "atomic"},
                        { common::service::transaction::Type::none, "none"},
                     };
                     return mapping[ common::service::transaction::mode( value.transaction)];
                  }
               };

               //
               // we need to set something when category is empty to help
               // enable possible use of sort, cut, awk and such
               auto format_category = []( const admin::ServiceVO& value){
                  if( value.category.empty())
                     return "-";
                  return value.category.c_str();
               };


               auto format_invoked = []( const admin::ServiceVO& value){
                  return value.metrics.count;
               };

               auto format_avg_time = []( const admin::ServiceVO& value){
                  if( value.metrics.count == 0)
                  {
                     return 0.0;
                  }

                  using second_t = std::chrono::duration< double>;
                  return std::chrono::duration_cast< second_t>( value.metrics.total / value.metrics.count).count();
               };


               auto format_pending_count = []( const admin::ServiceVO& value){
                  return value.pending.count;
               };

               auto format_avg_pending_time = []( const admin::ServiceVO& value){
                  if( value.metrics.count == 0)
                  {
                     return 0.0;
                  }

                  using second_t = std::chrono::duration< double>;
                  return std::chrono::duration_cast< second_t>( value.pending.total / value.metrics.count).count();
               };

               auto format_last = []( const admin::ServiceVO& value){
                  return common::chronology::local( value.last);
               };

               return terminal::format::formatter< admin::ServiceVO>::construct( 
                  terminal::format::column( "name", std::mem_fn( &admin::ServiceVO::name), terminal::color::yellow, terminal::format::Align::left),
                  terminal::format::column( "category", format_category, terminal::color::no_color, terminal::format::Align::left),
                  terminal::format::column( "mode", format_mode{}, terminal::color::no_color, terminal::format::Align::right),
                  terminal::format::column( "timeout", format_timeout{}, terminal::color::blue, terminal::format::Align::right),
                  terminal::format::column( "I", format::instance::local::total{}, terminal::color::white, terminal::format::Align::right),
                  terminal::format::column( "C", format_invoked, terminal::color::white, terminal::format::Align::right),
                  terminal::format::column( "AT", format_avg_time, terminal::color::white, terminal::format::Align::right),
                  terminal::format::column( "P", format_pending_count, terminal::color::magenta, terminal::format::Align::right),
                  terminal::format::column( "PAT", format_avg_pending_time, terminal::color::magenta, terminal::format::Align::right),
                  terminal::format::column( "RI", format::instance::remote::total{}, terminal::color::cyan, terminal::format::Align::right),
                  terminal::format::column( "RC", std::mem_fn( &admin::ServiceVO::remote_invocations), terminal::color::cyan, terminal::format::Align::right),
                  terminal::format::column( "last", format_last, terminal::color::blue, terminal::format::Align::right)
               );
            }


            auto instances()
            {
               using value_type = normalized::service::Instance;


               auto format_pid = []( auto& v){ return v.process.pid;};

               auto format_service_name = []( const value_type& v){
                  return v.service.get().name;
               };


               /*
               struct format_queue
               {
                  strong::ipc::id::type operator () ( const value_type& v) const { return v.process.ipc;}
               };
               */

               auto format_process_alias = []( const value_type& v) -> const std::string& {
                  if( v.executable) { return v.executable->alias;}
                  static std::string empty;
                  return empty;
               };


               struct format_state
               {
                  std::size_t width( const value_type& value, const std::ostream&) const
                  {
                     return 7;
                  }

                  void print( std::ostream& out, const value_type& value, std::size_t width, bool color) const
                  {
                     out << std::setfill( ' ');

                     if( color)
                     {
                        switch( value.state)
                        {
                           case value_type::State::idle: out << std::left << std::setw( width) << terminal::color::green << "idle"; break;
                           case value_type::State::busy: out << std::left << std::setw( width) << terminal::color::yellow << "busy"; break;
                           case value_type::State::remote: out << std::left << std::setw( width) << terminal::color::cyan << "remote"; break;
                           case value_type::State::exiting: out << std::left << std::setw( width) << terminal::color::magenta << "exiting"; break;
                           default: out << "unknown"; break;
                        }
                     }
                     else
                     {
                        switch( value.state)
                        {
                           case value_type::State::idle: out << std::left << std::setw( width) << "idle"; break;
                           case value_type::State::busy: out << std::left << std::setw( width) << "busy"; break;
                           case value_type::State::remote: out << std::left << std::setw( width) << "remote"; break;
                           case value_type::State::exiting: out << std::left << std::setw( width) << "exiting"; break;
                           default: out << "unknown"; break;
                        }
                     }
                  }
               };

               auto format_hops = []( const value_type& value){
                  return value.hops;
               };



               return terminal::format::formatter< normalized::service::Instance>::construct(
                  terminal::format::column( "service", format_service_name, terminal::color::yellow),
                  terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                  //terminal::format::column( "queue", format_queue{}, terminal::color::no_color, terminal::format::Align::right),
                  terminal::format::custom_column( "state", format_state{}),
                  terminal::format::column( "hops", format_hops, terminal::color::no_color, terminal::format::Align::right),

                  //terminal::format::column( "last", format_last{}, terminal::color::blue, terminal::format::Align::right),

                  terminal::format::column( "alias", format_process_alias, terminal::color::blue, terminal::format::Align::left)
               );
            }

         } // format


         namespace print
         {
            enum class Service
            {
               user,
               admin
            };

            void services( std::ostream& out, admin::StateVO& state, Service type)
            {

               auto split = algorithm::partition( state.services, []( const admin::ServiceVO& s){
                     return s.category != common::service::category::admin();});

               auto services = type == Service::user ? std::get< 0>( split) : std::get< 1>( split);

               algorithm::sort( services, []( const auto& l, const auto& r){ return l.name < r.name;});

               auto formatter = format::services( state);

               formatter.print( out, services);
            }

            template< typename IR>
            void instances( std::ostream& out, IR instances)
            {

               auto formatter = format::instances();

               formatter.print( out, instances);
            }

            void instances( std::ostream& out, call::State& state)
            {
               auto instances = normalized::service::instances( state);
               print::instances( out, instances);
            }

         } // print

         namespace action
         {

            auto services_completer = []( auto value, bool help)
            {
               if( help)
               {
                  return std::vector< std::string>{ "<service name>..."};
               }
               else
               {
                  auto state = admin::api::state();

                  return common::algorithm::transform( state.services, []( auto& service){
                     return std::move( service.name);
                  });
               }
            };

            void list_services()
            {
               auto state = admin::api::state();

               print::services( std::cout, state, print::Service::user);
            }

            void list_admin_services()
            {
               auto state = admin::api::state();

               print::services( std::cout, state, print::Service::admin);
            }

            void list_instances()
            {
               auto state = call::instances();

               print::instances( std::cout, state);
            }


            void output_state( const common::optional< std::string>& format)
            {
               auto state = admin::api::state();

               auto archive = serviceframework::archive::create::writer::from( format.value_or( ""), std::cout);
               archive << CASUAL_MAKE_NVP( state);
            }


            void list_service_legend()
            {
               std::cout << R"(legend for --list-services output: 
                         
    name:
       the name of the service
    category:
       arbitrary category to help understand the 'purpose' with the service
    mode: 
       transaction mode - can be one of the following (auto, join, none, atomic)
    timeout:
       the timeout for the service (in seconds)
    LI:
       Local-Instances number of local instances
    C:
       calls - number of calls to the service
    AT:
       average-time - the average time of the service
    P
       Pending - total number of pending request, over time.
    PAT
       Pending-Average-Time - the average time request has waited for a service to be available, over time.
    RI:
       Remote-Instances - number of remote instances
    RC:
       Remote-Calls - number of calls to remote instances (a subset of C)
    last:
       the last time the service was requested    

)";

            }

            namespace metric
            {

               void reset( const std::vector< std::string>& services)
               {
                  call::metric::reset( services);
               }

            } // metric

         } // action

         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  auto complete_state = []( auto values, bool){
                     return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                  };
                  return common::argument::Group{ [](){}, { "service"}, "service related administration",
                     common::argument::Option{ &action::list_services, { "-ls", "--list-services"}, "list services"},
                     common::argument::Option{ &action::list_service_legend, { "--list-services-legend"}, "legend for --list-services output"},
                     common::argument::Option{ &action::list_instances,  { "-li", "--list-instances"}, "list instances"},
                     common::argument::Option{ &action::metric::reset, action::services_completer,  { "-mr", "--metric-reset"}, "reset metrics for provided services, if no services provided, all metrics will be reset"},
                     common::argument::Option{ &action::list_admin_services,  { "--list-admin-services"}, "list casual administration services"},
                     common::argument::Option{ &action::output_state, complete_state, { "--state"}, "service state"},
                  };
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }
            
         } // admin 

      } // manager
   } // service
} // casual


