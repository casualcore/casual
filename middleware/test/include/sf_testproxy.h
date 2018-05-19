//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once



#include <serviceframework/service/protocol/call.h>

//## includes protected section begin [.10]
#include <string>
#include <vector>
//## includes protected section end   [.10]




namespace casual
{

namespace test
{

   //## declarations protected section begin [.20]
   //## declarations protected section end   [.20]


   // motsvarar FK-SF class ProxyName
   namespace proxyName
   {
      //## declarations protected section begin [.30]
      //## declarations protected section end   [.30]

      // tror vi k�r med f�rkortad asynchronous
      namespace send
      {
         //## declarations protected section begin [.100]
         //## declarations protected section end   [.100]

         //
         // Another design
         //
         class SomeService2
         {
         public:

            using Flag = casual::serviceframework::service::protocol::binary::Send::Flag;
            using Flags = casual::serviceframework::service::protocol::binary::Send::Flags;

            SomeService2();
            ~SomeService2();

            //## declarations protected section begin [666.100]
            //## declarations protected section end   [666.100]

            class Receive
            {
            public:
               using receive_type = casual::serviceframework::service::protocol::binary::Send::receive_type;
               using Flag = receive_type::Flag;
               using Flags = receive_type::Flags;

               std::vector< std::string> operator() ();

            private:
               friend class SomeService2;
               Receive( receive_type&& receive);
               receive_type m_receive;
            };


            Receive operator() ( const std::string& value);
            Receive operator() ( const std::string& value, Flags flags);

         };


         // fler services...

         //## declarations protected section begin [.110]
         //## declarations protected section end   [.110]

      } // send

      namespace call
      {
         using Flag = casual::serviceframework::service::protocol::binary::Call::Flag;
         using Flags = casual::serviceframework::service::protocol::binary::Call::Flags;

         // service name med inledande versal
         // comments genereras h�r.
         // Jag antar att Casual_sf_test1 har id 666 i detta exempel
         class SomeService2
         {
         public:

            SomeService2();
            ~SomeService2();

            //## declarations protected section begin [666.200]
            //## declarations protected section end   [666.200]

            std::vector< std::string> operator() ( const std::string& value);
            std::vector< std::string> operator() ( const std::string& value, Flags flags);
         };

         // fler services...


         //## declarations protected section begin [.210]
         //## declarations protected section end   [.210]

         std::vector< std::string> someService2( const std::string& value);
         std::vector< std::string> someService2( const std::string& value, Flags flags);

      } // call


      //## declarations protected section begin [.60]
      //## declarations protected section end   [.60]

      // comments


      // fler services...


      //## declarations protected section begin [.80]
      //## declarations protected section end   [.80]
   }



} // test

} // casual


