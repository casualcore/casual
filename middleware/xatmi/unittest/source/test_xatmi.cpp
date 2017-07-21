//!
//! casual
//!

#include "common/unittest.h"

#include "xatmi.h"



#include "common/mockup/ipc.h"
#include "common/mockup/transform.h"
#include "common/mockup/domain.h"

#include "common/flag.h"

#include "common/message/server.h"
#include "common/message/transaction.h"

#include <map>
#include <vector>

namespace casual
{
   using namespace common;

   namespace xatmi
   {
      namespace local
      {
         namespace
         {

            //
            // Represent a domain
            //
            struct Domain
            {
               Domain()
                  : server{ {
                     mockup::domain::echo::create::service( "service_1"),
                     mockup::domain::echo::create::service( "timeout_2"),
                     mockup::domain::echo::create::service( "service_urcode"), // will echo with urcode = 42

                     /*
                      * Will echo with error set to the following
                        #define TPEOS 7
                        #define TPEPROTO 9
                        #define TPESVCERR 10
                        #define TPESVCFAIL 11
                        #define TPESYSTEM 12
                      */
                     mockup::domain::echo::create::service( "service_TPEOS"),
                     mockup::domain::echo::create::service( "service_TPEPROTO"),
                     mockup::domain::echo::create::service( "service_TPESVCERR"),
                     mockup::domain::echo::create::service( "service_TPESVCFAIL"),
                     mockup::domain::echo::create::service( "service_TPESYSTEM"),
                  }}
               {

               }

               mockup::domain::Manager manager;
               mockup::domain::service::Manager service;
               mockup::domain::transaction::Manager tm;
               mockup::domain::echo::Server server;
            };

         } // <unnamed>
      } // local




      TEST( casual_xatmi, tpalloc_X_OCTET_binary__expect_ok)
      {
         common::unittest::Trace trace;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         EXPECT_TRUE( buffer != nullptr) << "tperrno: " << tperrno;
         tpfree( buffer);
      }

      TEST( casual_xatmi, tpacall_service_null__expect_TPEINVAL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         char buffer[ 100];
         EXPECT_TRUE( tpacall( nullptr, buffer, 0, 0) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrno;
      }



      TEST( casual_xatmi, tpacall_TPNOREPLY_without_TPNOTRAN__in_transaction___expect_TPEINVAL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         tx_begin();

         auto rollback = common::scope::execute( [](){
            tx_rollback();
         });

         char buffer[ 100];
         EXPECT_TRUE( tpacall( "some-service", buffer, 100, TPNOREPLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrno;


      }



      TEST( casual_xatmi, tpcancel_descriptor_42__expect_TPEBADDESC)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tpcancel( 42) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC) << "tperrno: " << tperrno;
      }


      TEST( casual_xatmi, tx_rollback__no_transaction__expect_TX_PROTOCOL_ERROR)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tx_rollback() == TX_PROTOCOL_ERROR);
      }


      TEST( casual_xatmi, tpgetrply_descriptor_42__expect_TPEBADDESC)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         int descriptor = 42;
         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpgetrply( &descriptor, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC) << "tperrno: " << tperrno;

         tpfree( buffer);
      }

      TEST( casual_xatmi, tpacall_service_XXX__expect_TPENOENT)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "XXX", buffer, 128, 0) == -1);
         EXPECT_TRUE( tperrno == TPENOENT) << "tperrno: " << tperrnostring( tperrno);


         tpfree( buffer);
      }

      TEST( casual_xatmi, tpacall_buffer_null__expect_expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto descriptor = tpacall( "service_1", nullptr, 0, 0);
         EXPECT_TRUE( descriptor != -1) << "tperrno: " << tperrno;
         EXPECT_TRUE( tpcancel( descriptor) != -1);
      }


      TEST( casual_xatmi, tpacall_service_service_1_TPNOREPLY_TPNOTRAN__expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "service_1", buffer, 128, TPNOREPLY | TPNOTRAN) == 0) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }

      TEST( casual_xatmi, tpacall_service_1_TPNOREPLY__no_current_transaction__expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "service_1", buffer, 128, TPNOREPLY ) == 0) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }

      TEST( casual_xatmi, tpacall_service_1_TPNOREPLY_ongoing_current_transaction__expect_TPEINVAL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "service_1", buffer, 128, TPNOREPLY ) == -1) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( tperrno == TPEINVAL);

         EXPECT_TRUE( tx_rollback() == TX_OK);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpcall_service_service_1__expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "service_1", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpacall_service_service_1__no_transaction__tpcancel___expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         auto descriptor = tpacall( "service_1", buffer, 128, 0);
         EXPECT_TRUE( descriptor == 1) << "desc: " << descriptor << " tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( tpcancel( descriptor) != -1)  << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpacall_service_service_1__10_times__no_transaction__tpcancel_all___expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "service_1", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << tperrnostring( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};
         EXPECT_TRUE( descriptors == expected_descriptors) << "descriptors: " << common::range::make( descriptors);

         for( auto& desc : descriptors)
         {
            EXPECT_TRUE( tpcancel( desc) != -1)  << "tperrno: " << tperrnostring( tperrno);
         }

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpacall_service_service_1__10_times___tpgetrply_any___expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;


         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "service_1", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << tperrnostring( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};


         std::vector< int> fetched( 10);

         for( auto& fetch : fetched)
         {
            auto len = tptypes( buffer, nullptr, nullptr);
            EXPECT_TRUE( tpgetrply( &fetch, &buffer, &len, TPGETANY) != -1)  << "tperrno: " << tperrnostring( tperrno);
         }

         EXPECT_TRUE( descriptors == fetched) << "descriptors: " << common::range::make( descriptors) << " fetched: " << common::range::make( fetched);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tx_begin__tpacall_service_service_1__10_times___tpgetrply_all__tx_commit__expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "service_1", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << tperrnostring( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};
         EXPECT_TRUE( descriptors == expected_descriptors) << "descriptors: " << common::range::make( descriptors);

         EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);

         for( auto& desc : descriptors)
         {
            auto len = tptypes( buffer, nullptr, nullptr);
            EXPECT_TRUE( tpgetrply( &desc, &buffer, &len, 0) != -1)  << "tperrno: " << tperrnostring( tperrno);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);

         tpfree( buffer);
      }

      TEST( casual_xatmi, tx_begin__tpcall_service_service_1__tx_commit___expect_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;


         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         auto len = tptypes( buffer, nullptr, nullptr);
         EXPECT_TRUE( tpcall( "service_1", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tx_commit() == TX_OK);

         tpfree( buffer);
      }

      TEST( casual_xatmi, tx_begin__tpacall_service_service_1__tx_commit___expect_TX_PROTOCOL_ERROR)
      {
         common::unittest::Trace trace;

         local::Domain domain;


         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "service_1", buffer, 128, 0) != 0) << "tperrno: " << tperrnostring( tperrno);

         // can't commit when there are pending replies
         EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);
         // we can always rollback the transaction
         EXPECT_TRUE( tx_rollback() == TX_OK);

         tpfree( buffer);
      }

      namespace local
      {
         namespace
         {

            bool call( const std::string& service)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, 128);
               auto len = tptypes( buffer, nullptr, nullptr);

               auto code = tpcall( service.c_str(), buffer, 128, &buffer, &len, 0);

               tpfree( buffer);

               return code == 0;
            }


         } // <unnamed>
      } // local

      TEST( casual_xatmi, tpcall_service_urcode__expect_ok__urcode_42)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::call( "service_urcode")) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( tpurcode == 42) << "urcode: " << tpurcode;
      }


      /*
       *                      mockup::domain::echo::create::service( "service_TPEOS"),
                     mockup::domain::echo::create::service( "service_TPEPROTO"),
                     mockup::domain::echo::create::service( "service_TPESVCERR"),
                     mockup::domain::echo::create::service( "service_TPESVCFAIL"),
                     mockup::domain::echo::create::service( "service_TPESYSTEM"),
       */

      TEST( casual_xatmi, tpcall_service_TPEOS___expect_error_TPEOS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_FALSE( local::call( "service_TPEOS"));
         EXPECT_TRUE( tperrno == TPEOS) << "tperrno: " << tperrno;
      }

      TEST( casual_xatmi, tpcall_service_TPEPROTO___expect_error_TPEPROTO)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_FALSE( local::call( "service_TPEPROTO"));
         EXPECT_TRUE( tperrno == TPEPROTO) << "tperrno: " << tperrno;
      }

      TEST( casual_xatmi, tpcall_service_TPESVCERR___expect_error_TPESVCERR)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_FALSE( local::call( "service_TPESVCERR"));
         EXPECT_TRUE( tperrno == TPESVCERR) << "tperrno: " << tperrno;
      }

      TEST( casual_xatmi, tpcall_service_TPESYSTEM___expect_error_TPESYSTEM)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_FALSE( local::call( "service_TPESYSTEM"));
         EXPECT_TRUE( tperrno == TPESYSTEM) << "tperrno: " << tperrno;
      }

      TEST( casual_xatmi, tpcall_service_TPESVCFAIL___expect_error_TPESVCFAIL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_FALSE( local::call( "service_TPESVCFAIL"));
         EXPECT_TRUE( tperrno == TPESVCFAIL) << "tperrno: " << tperrno;
      }

      /*
      TEST( casual_xatmi, tpcall_service_timeout_2__expect_TPETIME)
      {
         //
         // Set up a "linked-domain" that transforms request to replies - see above
         //
         local::Domain domain;

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         long size = 0;

         EXPECT_TRUE( tpcall( "timeout_2", buffer, 128, &buffer, &size, 0) == -1);
         EXPECT_TRUE( tperrno == TPETIME) << "tperrno: " << common::error::xatmi::error( tperrno);

         tpfree( buffer);
      }
      */


   } // xatmi
} // casual
