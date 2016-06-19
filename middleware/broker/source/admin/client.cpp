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



      namespace format
      {


         struct base_instances
         {
            base_instances( const std::vector< admin::InstanceVO>& instances) : m_instances( instances) {}

         protected:

            std::vector< admin::InstanceVO> instances( const std::vector< platform::pid::type>& pids) const
            {
               std::vector< admin::InstanceVO> result;

               for( auto& pid : pids)
               {
                  auto found = range::find_if( m_instances, [=]( const admin::InstanceVO& v){
                     return v.process.pid == pid;
                  });
                  if( found)
                  {
                     result.push_back( *found);
                  }
               }

               return result;
            }

            const std::vector< admin::InstanceVO>& m_instances;
         };

         struct format_state :  base_instances
         {
            using base_instances::base_instances;

            template< typename T>
            std::size_t width( const T& value) const
            {
               return value.instances.size();
            }



            template< typename T>
            void print( std::ostream& out, const T& value, std::size_t width, bool color) const
            {
               if( color)
               {
                  for( auto& instance : instances( value.instances))
                  {
                     switch( instance.state)
                     {
                        case admin::InstanceVO::State::idle: out << terminal::color::green.start() << '+'; break;
                        case admin::InstanceVO::State::busy: out << terminal::color::yellow.start() << '*'; break;
                        case admin::InstanceVO::State::remote: out << terminal::color::magenta.start() << '-'; break;
                        default: out << terminal::color::red.start() <<  '?'; break;
                     }
                  }
                  out << terminal::color::green.end();
               }
               else
               {
                  for( auto& instance : instances( value.instances))
                  {
                     switch( instance.state)
                     {
                        case admin::InstanceVO::State::idle: out << '+'; break;
                        case admin::InstanceVO::State::busy: out << '*'; break;
                        case admin::InstanceVO::State::remote: out << '-'; break;
                        default: out <<  '?'; break;
                     }
                  }
               }

               //
               // Pad manually
               //

               if( width != 0 )
               {
                  out << std::string( width - value.instances.size(), ' ');
               }
            }
         };


         struct format_instances
         {
            template< typename T>
            std::size_t operator () ( const T& value) const { return value.instances.size();}
         };

         terminal::format::formatter< admin::ServiceVO> services( const std::vector< admin::InstanceVO>& instances)
         {

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
                  static std::map< server::Service::Transaction, const char*> mapping{
                     {server::Service::Transaction::automatic, "auto"},
                     {server::Service::Transaction::join, "join"},
                     {server::Service::Transaction::atomic, "atomic"},
                     {server::Service::Transaction::none, "none"},
                  };
                  return mapping[ server::Service::Transaction( value.transaction)];
               }
            };

            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "name", std::mem_fn( &admin::ServiceVO::name), terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "type", std::mem_fn( &admin::ServiceVO::type), terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "mode", format_mode{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "timeout", format_timeout{}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "requested", std::mem_fn( &admin::ServiceVO::lookedup), terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "#", format_instances{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{ instances})
            };
         }


         terminal::format::formatter< admin::InstanceVO> instances()
         {


            struct format_pid
            {
               platform::pid::type operator () ( const admin::InstanceVO& v) const { return v.process.pid;}
            };

            struct format_queue
            {
               platform::ipc::id::type operator () ( const admin::InstanceVO& v) const { return v.process.queue;}
            };


            struct format_state
            {
               std::size_t width( const admin::InstanceVO& value) const
               {
                  return 6;
               }

               void print( std::ostream& out, const admin::InstanceVO& value, std::size_t width, bool color) const
               {
                  out << std::setfill( ' ');

                  if( color)
                  {
                     switch( value.state)
                     {
                        case admin::InstanceVO::State::idle: out << std::right << std::setw( width) << terminal::color::green << "idle"; break;
                        case admin::InstanceVO::State::busy: out << std::right << std::setw( width) << terminal::color::yellow << "busy"; break;
                        case admin::InstanceVO::State::remote: out << std::right << std::setw( width) << terminal::color::magenta << "remote"; break;
                     }
                  }
                  else
                  {
                     switch( value.state)
                     {
                        case admin::InstanceVO::State::idle: out << std::right << std::setw( width) << "idle"; break;
                        case admin::InstanceVO::State::busy: out << std::right << std::setw( width) << "busy"; break;
                        case admin::InstanceVO::State::remote: out  << std::right << std::setw( width) << "remote"; break;
                     }
                  }
               }
            };

            struct format_last
            {
               std::string operator () ( const admin::InstanceVO& v) const { return chronology::local( v.last);}
            };


            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "pid", format_pid{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "queue", format_queue{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{}),
               terminal::format::column( "invoked", std::mem_fn( &admin::InstanceVO::invoked), terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "last", format_last{}, terminal::color::blue, terminal::format::Align::right),
            };
         }

      } // format


      namespace print
      {


         void services( std::ostream& out, admin::StateVO& state)
         {
            range::sort( state.services, []( const admin::ServiceVO& l, const admin::ServiceVO& r){ return l.name < r.name;});

            auto formatter = format::services( state.instances);

            formatter.print( std::cout, std::begin( state.services), std::end( state.services));
         }

         template< typename IR>
         void instances( std::ostream& out, IR instances_range)
         {
            range::sort( instances_range, []( const admin::InstanceVO& l, const admin::InstanceVO& r){ return l.process < r.process;});

            auto formatter = format::instances();

            formatter.print( std::cout, instances_range);
         }

         void instances( std::ostream& out, admin::StateVO& state)
         {
            instances( out, state.instances);
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


