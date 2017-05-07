//!
//! casual
//!


#include "sf/xatmi_call.h"
#include "sf/namevaluepair.h"
#include "sf/archive/log.h"
#include "sf/archive/maker.h"

#include "broker/admin/brokervo.h"
#include "domain/manager/admin/vo.h"

#include "common/file.h"
#include "common/arguments.h"
#include "common/chronology.h"
#include "common/terminal.h"
#include "common/server/service.h"


#include "broker/broker.h"

//
// std
//
#include <iostream>
#include <iomanip>
#include <limits>




namespace casual
{

   using namespace common;

   namespace broker
   {

      namespace global
      {
         bool porcelain = false;

         bool no_colors = false;
         bool no_header = false;

         bool admin_services = false;

      } // global



      namespace call
      {
         admin::StateVO state()
         {
            sf::xatmi::service::binary::Sync service( ".casual.broker.state");
            auto reply = service();

            admin::StateVO serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }

         struct State
         {
            admin::StateVO broker;
            casual::domain::manager::admin::vo::State domain;
         };

         State instances()
         {
            sf::xatmi::service::binary::Async service{ ".casual.domain.state"};
            auto reply = service();

            State result;
            result.broker = call::state();

            reply() >> CASUAL_MAKE_NVP( result.domain);

            return result;
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

            range::copy( state.instances.local, std::back_inserter( result));
            range::copy( state.instances.remote, std::back_inserter( result));

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

               struct
               {
                  std::size_t invoked = 0;
                  std::size_t hops = 0;

               } services;

               std::size_t invoked = 0;

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
                     const casual::domain::manager::admin::vo::Executable* executable( const call::State& state, platform::pid::type pid)
                     {
                        auto found = range::find_if( state.domain.executables, [=]( const casual::domain::manager::admin::vo::Executable& e){
                           return range::find( e.instances, pid);
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

               for( auto& service : state.broker.services)
               {
                  for( auto& i : service.instances.local)
                  {
                     auto local = range::find( state.broker.instances.local, i.pid);
                     Instance instance{ service};
                     instance.invoked = local->invoked;
                     instance.state = local->state == admin::instance::LocalVO::State::idle ? Instance::State::idle : Instance::State::busy;
                     instance.process.pid = i.pid;
                     instance.services.invoked = i.metrics.invoked;
                     instance.executable = local::lookup::executable( state, i.pid);
                     result.push_back( std::move( instance));
                  }

                  for( auto& i : service.instances.remote)
                  {
                     auto remote = range::find( state.broker.instances.remote, i.pid);
                     Instance instance{ service};
                     instance.invoked = remote->invoked;
                     instance.process.pid = i.pid;
                     instance.services.invoked = i.invoked;
                     instance.services.hops = i.hops;
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

                  return std::get< 0>( range::intersection( m_instances, instances, []( const normalized::Instance& ni, const T& i){
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

                     return range::count_if( instances, []( const normalized::Instance& i){
                        return i.state == normalized::Instance::State::busy;
                     });
                  }
               };

               struct pending_base : state_base
               {
                  using state_base::state_base;

                  std::size_t pending( const std::string& service) const
                  {
                     return range::count_if( get().pending, [&]( const admin::PendingVO& p){
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

               struct load : pending_base, base_instances
               {
                  load( const admin::StateVO& state, std::vector< normalized::Instance>& instances)
                   : pending_base{ state}, base_instances{ instances} {}

                  double operator () ( const admin::ServiceVO& value) const
                  {
                     auto total = instance::local::total{}( value);
                     double load = instance::local::busy{ m_instances}( value) +
                           range::count_if( get().pending, [&]( const admin::PendingVO& p){
                              return p.requested == value.name;
                           });

                     if( total > 0)
                     {
                        return load / total;
                     }
                     return load;
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

         terminal::format::formatter< admin::ServiceVO> services( const admin::StateVO& state)
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
                  static std::map< service::transaction::Type, const char*> mapping{
                     { service::transaction::Type::automatic, "auto"},
                     { service::transaction::Type::join, "join"},
                     { service::transaction::Type::atomic, "atomic"},
                     { service::transaction::Type::none, "none"},
                  };
                  return mapping[ service::transaction::mode( value.transaction)];
               }
            };


            auto format_invoked = []( const admin::ServiceVO& value){
               auto invoked = value.metrics.invoked;

               range::for_each( value.instances.local, [&]( const admin::service::instance::Local& i){
                  invoked += i.metrics.invoked;
               });

               range::for_each( value.instances.remote, [&]( const admin::service::instance::Remote& i){
                  invoked += i.invoked;
               });

               return invoked;
            };

            auto format_avg_time = []( const admin::ServiceVO& value){
               auto metrics = value.metrics;

               range::for_each( value.instances.local, [&]( const admin::service::instance::Local& i){
                  metrics.invoked += i.metrics.invoked;
                  metrics.total += i.metrics.total;
               });

               if( metrics.invoked == 0)
               {
                  return 0.0;
               }

               using second_t = std::chrono::duration< double>;
               return std::chrono::duration_cast< second_t>( metrics.total / metrics.invoked).count();
            };


            auto format_pending_count = []( const admin::ServiceVO& value){
               return value.pending.count;
            };

            auto format_avg_pending_time = []( const admin::ServiceVO& value){
               auto invoked = value.metrics.invoked;

               range::for_each( value.instances.local, [&]( const admin::service::instance::Local& i){
                  invoked += i.metrics.invoked;
               });

               if( invoked == 0)
               {
                  return 0.0;
               }

               using second_t = std::chrono::duration< double>;
               return std::chrono::duration_cast< second_t>( value.pending.total / invoked).count();
            };

            auto format_last = []( const admin::ServiceVO& value){
               auto last = value.metrics.last;

               for( auto& instance : value.instances.local)
               {
                  last = std::max( last, instance.metrics.last);
               }

               for( auto& instance : value.instances.remote)
               {
                  last = std::max( last, instance.last);
               }
               return common::chronology::local( last);
            };

            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "name", std::mem_fn( &admin::ServiceVO::name), terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "category", std::mem_fn( &admin::ServiceVO::category), terminal::color::no_color, terminal::format::Align::left),
               terminal::format::column( "mode", format_mode{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "timeout", format_timeout{}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "invoked", format_invoked, terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "local", format::instance::local::total{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "load", format::instance::local::load{ state, instances}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "avg T", format_avg_time, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "tot pending #", format_pending_count, terminal::color::magenta, terminal::format::Align::right),
               terminal::format::column( "avg pending T", format_avg_pending_time, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "remote", format::instance::remote::total{}, terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "last", format_last, terminal::color::blue, terminal::format::Align::right),
            };
         }


         terminal::format::formatter< normalized::service::Instance> instances()
         {
            using value_type = normalized::service::Instance;


            struct format_pid
            {
               platform::pid::type operator () ( const value_type& v) const { return v.process.pid;}
            };

            auto format_service_name = []( const value_type& v){
               return v.service.get().name;
            };


            /*
            struct format_queue
            {
               platform::ipc::id::type operator () ( const value_type& v) const { return v.process.queue;}
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
               return value.services.hops;
            };

            auto format_service_invoke = []( const value_type& value){
               return value.services.invoked;
            };

            auto format_service_percent = []( const value_type& value){

               if( value.invoked > 0)
               {
                  return value.services.invoked / static_cast< double>( value.invoked);
               }
               return 0.0;
            };

            /*
            struct format_last
            {
               std::string operator () ( const value_type& v) const { return chronology::local( v..last);}
            };
            */


            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "service", format_service_name, terminal::color::yellow),
               terminal::format::column( "pid", format_pid{}, terminal::color::white, terminal::format::Align::right),
               //terminal::format::column( "queue", format_queue{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{}),
               terminal::format::column( "hops", format_hops, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "invoked", format_service_invoke, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "total", std::mem_fn( &value_type::invoked), terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "ratio", format_service_percent, terminal::color::no_color, terminal::format::Align::right),
               //terminal::format::column( "last", format_last{}, terminal::color::blue, terminal::format::Align::right),

               terminal::format::column( "alias", format_process_alias, terminal::color::blue, terminal::format::Align::left),

            };
         }

      } // format


      namespace print
      {


         void services( std::ostream& out, admin::StateVO& state)
         {
            auto services = range::make( state.services);

            if( ! broker::global::admin_services)
            {
               services = std::get< 0>( range::partition( services, []( const admin::ServiceVO& s){
                  return s.category != common::service::category::admin;}));
            }

            range::sort( services, []( const admin::ServiceVO& l, const admin::ServiceVO& r){ return l.name < r.name;});

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
         std::ostream& set_format( std::ostream& out)
         {
            return out << std::fixed << std::setprecision( 4);
         }


         void list_services()
         {
            auto state = call::state();

            print::services( set_format( std::cout), state);
         }

         void list_instances()
         {
            auto state = call::instances();

            print::instances( set_format( std::cout), state);
         }


         void output_state( const std::string& format)
         {
            auto archive = sf::archive::writer::from::name( std::cout, format);

            auto state = call::state();

            archive << CASUAL_MAKE_NVP( state);
         }


      } // action

   } // broker
} // casual



int main( int argc, char** argv)
{

   casual::common::Arguments parser{ {
         casual::common::argument::directive( {"--porcelain"}, "easy to parse format", casual::broker::global::porcelain),
         casual::common::argument::directive( {"--no-color"}, "no color will be used", casual::broker::global::no_colors),
         casual::common::argument::directive( {"--no-header"}, "no descriptive header for each column will be used", casual::broker::global::no_header),
         casual::common::argument::directive( {"--admin"}, "casual administration services will be included", casual::broker::global::admin_services),
         casual::common::argument::directive( {"-ls", "--list-services"}, "list services", &casual::broker::action::list_services),
         casual::common::argument::directive( {"-li", "--list-instances"}, "list instances", &casual::broker::action::list_instances),
         casual::common::argument::directive( {"-s", "--state"}, "prints the state on stdout in the provided format (json|yaml|xml|ini)", &casual::broker::action::output_state),

      }
   };


   try
   {
      parser.parse( argc, argv);

   }
   catch( const std::exception& exception)
   {
      std::cerr << "error: " << exception.what() << std::endl;
   }


   return 0;
}


