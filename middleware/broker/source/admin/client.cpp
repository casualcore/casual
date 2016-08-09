//!
//! casual
//!


#include "sf/xatmi_call.h"
#include "sf/namevaluepair.h"
#include "sf/archive/log.h"

#include "broker/admin/brokervo.h"

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

            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "name", std::mem_fn( &admin::ServiceVO::name), terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "type", std::mem_fn( &admin::ServiceVO::type), terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "mode", format_mode{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "timeout", format_timeout{}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "requested", std::mem_fn( &admin::ServiceVO::lookedup), terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "local", format::instance::local::total{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "busy", format::instance::local::busy{ instances}, terminal::color::red, terminal::format::Align::right),
               terminal::format::column( "pending", format::instance::local::pending{ state}, terminal::color::yellow, terminal::format::Align::right),
               terminal::format::column( "load", format::instance::local::load{ state, instances}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "remote", format::instance::remote::total{}, terminal::color::cyan, terminal::format::Align::right),
            };
         }


         terminal::format::formatter< normalized::Instance> instances()
         {
            using value_type = normalized::Instance;


            struct format_pid
            {
               platform::pid::type operator () ( const value_type& v) const { return v.process.pid;}
            };

            struct format_queue
            {
               platform::ipc::id::type operator () ( const value_type& v) const { return v.process.queue;}
            };



            struct format_state
            {
               std::size_t width( const value_type& value) const
               {
                  return 6;
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
                     }
                  }
                  else
                  {
                     switch( value.state)
                     {
                        case value_type::State::idle: out << std::left << std::setw( width) << "idle"; break;
                        case value_type::State::busy: out << std::left << std::setw( width) << "busy"; break;
                        case value_type::State::remote: out << std::left << std::setw( width) << "remote"; break;
                     }
                  }
               }
            };

            struct format_last
            {
               std::string operator () ( const normalized::Instance& v) const { return chronology::local( v.last);}
            };


            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "pid", format_pid{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "queue", format_queue{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{}),
               terminal::format::column( "invoked", std::mem_fn( &admin::instance::Base::invoked), terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "last", format_last{}, terminal::color::blue, terminal::format::Align::right),
            };
         }

      } // format


      namespace print
      {


         void services( std::ostream& out, admin::StateVO& state)
         {
            range::sort( state.services, []( const admin::ServiceVO& l, const admin::ServiceVO& r){ return l.name < r.name;});

            auto formatter = format::services( state);

            formatter.print( std::cout, std::begin( state.services), std::end( state.services));
         }

         template< typename IR>
         void instances( std::ostream& out, IR instances_range)
         {
            range::sort( instances_range, []( const admin::instance::Base& l, const admin::instance::Base& r){ return l.process < r.process;});

            auto formatter = format::instances();

            formatter.print( std::cout, instances_range);
         }

         void instances( std::ostream& out, admin::StateVO& state)
         {
            instances( out, normalized::instances( state));
         }

      } // print

      namespace action
      {


         void list_services()
         {
            auto state = call::state();

            print::services( std::cout, state);
         }

         void list_instances()
         {
            auto state = call::state();

            print::instances( std::cout, state);
         }

         void list_pending()
         {

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
         casual::common::argument::directive( {"-ls", "--list-services"}, "list services", &casual::broker::action::list_services),
         casual::common::argument::directive( {"-li", "--list-instances"}, "list instances", &casual::broker::action::list_instances),
         casual::common::argument::directive( {"-lp", "--list-pending"}, "list pending service call", &casual::broker::action::list_pending),
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


