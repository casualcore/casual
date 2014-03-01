

#ifndef SF_TESTPROXY_H_
#define SF_TESTPROXY_H_

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

      // tror vi kör med förkortad asynchronous
      namespace async
      {
         //## declarations protected section begin [.100]
         //## declarations protected section end   [.100]


         // service name med inledande versal
         // comments genereras här.
         // Jag antar att Casual_sf_test1 har id 666 i detta exempel
         class Casual_sf_test1 : public casual::sf::proxy::async
         {
         public:

            using casual::sf::proxy::async::async;

            //## declarations protected section begin [666.100]
            //## declarations protected section end   [666.100]


            // (in, inout)
            //  vi skulle kunna splitta comments om arguemt baserat på in, out osv
            void send( const std::string);

            // return, (out, inout)
            //  vi skulle kunna splitta comments om arguemt baserat på in, out osv
            std::vector< std::string> receive();
         };

         // fler services...

         //## declarations protected section begin [.110]
         //## declarations protected section end   [.110]

      } // asynk

      namespace sync
      {
         // service name med inledande versal
         // comments genereras här.
         // Jag antar att Casual_sf_test1 har id 666 i detta exempel
         class Casual_sf_test1 : public casual::sf::proxy::sync
         {
         public:

            using casual::sf::proxy::sync::sync;

            //## declarations protected section begin [666.200]
            //## declarations protected section end   [666.200]

            std::vector< std::string> call( const std::string& value);
         };

         // fler services...


         //## declarations protected section begin [.210]
         //## declarations protected section end   [.210]

      } // sync


      //## declarations protected section begin [.60]
      //## declarations protected section end   [.60]

      // comments
      std::vector< std::string> casual_sf_test1( const std::string& value);

      // fler services...


      //## declarations protected section begin [.80]
      //## declarations protected section end   [.80]
   };



} // test

} // casual

#endif // SF_TESTPROXY_H_
