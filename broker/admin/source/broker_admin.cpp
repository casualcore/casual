//!
//! broker_admin.cpp
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!


#include "sf/xatmi_call.h"

#include "broker/brokervo.h"

#include "common/file.h"
#include "common/arguments.h"
#include "common/chronology.h"


//
// std
//
#include <iostream>
#include <iomanip>
#include <limits>
#include "broker/broker.h"



namespace casual
{

   namespace broker
   {

      struct Print
      {
         void operator () ( const admin::InstanceVO& value) const
         {
            std::cout << std::setfill( ' ') <<
                  std::setw( 10) << value.pid <<
                  std::setw( 12) << value.queueId << std::setw( 8);

            switch( static_cast< Server::Instance::State>( value.state))
            {
               case Server::Instance::State::absent: std::cout << "absent"; break;
               case Server::Instance::State::prospect: std::cout << "prospect"; break;
               case Server::Instance::State::idle: std::cout << "idle"; break;
               case Server::Instance::State::busy: std::cout << "busy"; break;
               case Server::Instance::State::shutdown: std::cout << "shutdown"; break;
            }


           std::cout << std::setw( 10) << std::right << value.invoked << std::left << "  " << common::chronology::local( value.last) <<  std::endl;
         }

         void operator () ( const admin::ServerVO& value) const
         {
            std::cout <<  std::setw( 20) << std::setfill( ' ') << std::left << value.alias << " path: " << value.path << std::endl;

            std::cout << "  |- PID       QUEUE       STATE      INVOKED  TIMESTAMP" << std::endl;
            for( auto& instance : value.instances)
            {
               std::cout << "  |- ";
               Print{}( instance);
            }
         }

         void operator () ( const admin::ServiceVO& value) const
         {
            std::cout <<  std::setw( 32) << std::setfill( ' ') << std::left << value.name <<
                  std::setw( 12) << std::right << value.instances.size() <<
                  std::setw( 14) << std::right << value.lookedup <<
                  std::setw( 10) << value.timeout << std::endl;
         }

      };


      void listServers()
      {
         sf::xatmi::service::binary::Sync service( "_broker_listServers");
         auto reply = service();

         std::vector< admin::ServerVO> serviceReply;

         reply >> CASUAL_MAKE_NVP( serviceReply);

         std::for_each( std::begin( serviceReply), std::end( serviceReply), Print() );

      }

      void listServices()
      {
         sf::xatmi::service::binary::Sync service( "_broker_listServices");
         auto reply = service();

         std::vector< admin::ServiceVO> serviceReply;

         reply >> CASUAL_MAKE_NVP( serviceReply);


         std::cout << "NAME                               INSTANCES        CALLED   TIMEOUT" << std::endl;
         std::for_each( std::begin( serviceReply), std::end( serviceReply), Print() );

      }

      // TODO: Test
      void listServicesJSON()
      {
         sf::buffer::X_Octet input( "JSON");
         input.str( "{}");
         sf::buffer::X_Octet output( "JSON");

         sf::xatmi::service::call( "_broker_listServers", input, output, 0);

         auto raw = output.raw();
         const std::string json( raw.buffer, raw.buffer + raw.size);

         std::cout << "json:\n" << json << std::endl;


      }


      void updateInstances( const std::vector< std::string>& values)
      {
         if( values.size() == 2)
         {
            admin::update::InstancesVO instance;

            instance.alias = values[ 0];
            instance.instances = std::stoul( values[ 1]);

            sf::xatmi::service::binary::Sync service( "_broker_updateInstances");

            service << CASUAL_MAKE_NVP( std::vector< admin::update::InstancesVO>{ instance});

            service();

         }
      }


   }

}



int usage( int argc, char** argv)
{
   std::cerr << "usage:\n  " << argv[ 0] << " (--list-servers | --list-services | -update-instances)" << std::endl;
   return 2;
}



int main( int argc, char** argv)
{

   casual::common::Arguments parser;
   parser.add(
         casual::common::argument::directive( {"-lsvr", "--list-servers"}, "list all servers", &casual::broker::listServers),
         casual::common::argument::directive( {"-lsvc", "--list-services"}, "list all services", &casual::broker::listServices),
         casual::common::argument::directive( {"-usi", "--update-instances"}, "<alias> <#> update server instances", &casual::broker::updateInstances),
         casual::common::argument::directive( {"-lsvr-json", "--list-servers-json"}, "list all servers", &casual::broker::listServicesJSON)
   );




   try
   {
      parser.parse( argc, argv);

   }
   catch( const std::exception& exception)
   {
      std::cerr << "exception: " << exception.what() << std::endl;
   }


   return 0;
}


