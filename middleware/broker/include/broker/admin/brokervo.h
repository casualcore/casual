//!
//! casual
//!

#ifndef BROKERVO_H_
#define BROKERVO_H_
#include "sf/namevaluepair.h"
#include "sf/platform.h"


namespace casual
{
   namespace broker
   {
      namespace admin
      {
         namespace instance
         {
            struct Base
            {
               common::process::Handle process;
               std::size_t invoked = 0;
               sf::platform::time_point last;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( process);
                  archive & CASUAL_MAKE_NVP( invoked);
                  archive & CASUAL_MAKE_NVP( last);
               })

               friend bool operator < ( const Base& lhs, const Base& rhs) { return lhs.process.pid < rhs.process.pid;}
               friend bool operator == ( const Base& lhs, const Base& rhs) { return lhs.process.pid == rhs.process.pid;}
               friend bool operator == ( const Base& lhs, common::platform::pid::type rhs) { return lhs.process.pid == rhs;}

            };

            struct LocalVO : Base
            {

               enum class State : char
               {
                  idle,
                  busy,
               };

               State state = State::busy;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  Base::serialize( archive);
                  archive & CASUAL_MAKE_NVP( state);
               })
            };

            struct RemoteVO : Base
            {

            };

         } // instance


         namespace service
         {
            namespace pending
            {
               struct Metric
               {
                  std::size_t count = 0;
                  std::chrono::microseconds total;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( count);
                     archive & CASUAL_MAKE_NVP( total);
                  })
               };

            } // pending

            struct Metric
            {
               std::size_t invoked = 0;
               sf::platform::time_point last;
               std::chrono::microseconds total;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( invoked);
                  archive & CASUAL_MAKE_NVP( last);
                  archive & CASUAL_MAKE_NVP( total);
               })
            };

            namespace instance
            {

               struct Local
               {
                  sf::platform::pid::type pid;
                  Metric metrics;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( pid);
                     archive & CASUAL_MAKE_NVP( metrics);
                  })
               };

               struct Remote
               {
                  sf::platform::pid::type pid;
                  std::size_t invoked;
                  std::size_t hops;
                  sf::platform::time_point last;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( pid);
                     archive & CASUAL_MAKE_NVP( invoked);
                     archive & CASUAL_MAKE_NVP( hops);
                     archive & CASUAL_MAKE_NVP( last);
                  })
               };

            } // instance

         } // service


         struct ServiceVO
         {
            std::string name;
            std::chrono::microseconds timeout;
            std::size_t type = 0;
            std::size_t transaction = 0;

            service::Metric metrics;
            service::pending::Metric pending;


            struct
            {
               std::vector< service::instance::Local> local;
               std::vector< service::instance::Remote> remote;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( local);
                  archive & CASUAL_MAKE_NVP( remote);
               })
            } instances;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( timeout);
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( transaction);
               archive & CASUAL_MAKE_NVP( metrics);
               archive & CASUAL_MAKE_NVP( pending);
               archive & CASUAL_MAKE_NVP( instances);
            })
         };

         struct PendingVO
         {
            std::string requested;
            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( requested);
               archive & CASUAL_MAKE_NVP( process);
            })
         };

         struct StateVO
         {
            struct
            {
               std::vector< instance::LocalVO> local;
               std::vector< instance::RemoteVO> remote;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( local);
                  archive & CASUAL_MAKE_NVP( remote);
               })

            } instances;


            std::vector< ServiceVO> services;
            std::vector< PendingVO> pending;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( services);
               archive & CASUAL_MAKE_NVP( pending);
            })

         };

      } // admin
   } // broker


} // casual

#endif // BROKERVO_H_
