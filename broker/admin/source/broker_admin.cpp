//!
//! broker_admin.cpp
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!


#include "sf/xatmi_call.h"

#include "broker/servervo.h"
#include "broker/servicevo.h"

#include "common/file.h"


//
// std
//
#include <iostream>
#include <iomanip>
#include <limits>

namespace casual
{

   namespace broker
   {

      struct Print
      {
         void operator () ( const admin::ServerVO& value) const
         {
            std::cout <<  std::setw( 20) << std::left  << common::file::basename( value.getPath()) <<
                  std::setw( 10) << value.getPid() <<
                  std::setw( 12) << value.getQueue() << std::setw( 8) << ( value.getIdle() ? "idle" : "busy") << std::endl;
         }

         void operator () ( const admin::ServiceVO& value) const
         {
            std::cout <<  std::setw( 32) << std::left << value.getNameF() <<
                  std::setw( 12) << value.getPids().size() <<
                  std::setw( 12) << value.getTimeoutF() << std::endl;
         }

      };


      void listServers()
      {
         sf::xatmi::service::binary::Sync<> service( "_broker_listServers");
         auto reply = service.call();

         std::vector< admin::ServerVO> serviceReply;

         reply >> CASUAL_MAKE_NVP( serviceReply);

         std::cout << "NAME                PID       QUEUE       STATE" << std::endl;
         std::for_each( std::begin( serviceReply), std::end( serviceReply), Print() );

      }

      void listServices()
      {
         sf::xatmi::service::binary::Sync<> service( "_broker_listServices");
         auto reply = service.call();

         std::vector< admin::ServiceVO> serviceReply;

         reply >> CASUAL_MAKE_NVP( serviceReply);


         std::cout << "NAME                            INSTANCES   TIMEOUT" << std::endl;
         std::for_each( std::begin( serviceReply), std::end( serviceReply), Print() );

      }



   }

}



int usage( int argc, char** argv)
{
   std::cerr << "usage:\n  " << argv[ 0] << " (--list-servers | --list-services)" << std::endl;
   return 2;
}


int main( int argc, char** argv)
{

   if( argc != 2)
   {
      return usage( argc, argv);
   }

   const std::string option{ argv[ 1]};

   try
   {
      if( option == "--list-servers")
      {
         casual::broker::listServers();
      }
      else if( option == "--list-services")
      {
         casual::broker::listServices();
      }
      else
      {
         return usage( argc, argv);
      }


   }
   catch( const std::exception& exception)
   {
      std::cerr << "exception: " << exception.what() << std::endl;
   }




   return 0;
}


