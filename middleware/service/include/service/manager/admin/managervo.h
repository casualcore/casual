//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_SERVICE_MANAGER_ADMIN_MANAGERVO_H_
#define CASUAL_SERVICE_MANAGER_ADMIN_MANAGERVO_H_

#include "sf/namevaluepair.h"
#include "sf/platform.h"


namespace casual
{
   namespace service
   {
      namespace manager
      {
         namespace admin
         {
            namespace instance
            {
               struct Base
               {
                  common::process::Handle process;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( process);
                  })

                  friend bool operator < ( const Base& lhs, const Base& rhs) { return lhs.process.pid < rhs.process.pid;}
                  friend bool operator == ( const Base& lhs, const Base& rhs) { return lhs.process.pid == rhs.process.pid;}
                  friend bool operator == ( const Base& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

               };

               struct LocalVO : Base
               {

                  enum class State : char
                  {
                     idle,
                     busy,
                     exiting,
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

               struct Metric
               {
                  sf::platform::size::type count = 0;
                  std::chrono::nanoseconds total;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( count);
                     archive & CASUAL_MAKE_NVP( total);
                  })
               };

               namespace instance
               {

                  struct Local
                  {
                     sf::strong::process::id pid;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( pid);
                     })
                  };

                  struct Remote
                  {
                     sf::strong::process::id pid;
                     sf::platform::size::type hops;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( pid);
                        archive & CASUAL_MAKE_NVP( hops);
                     })
                  };

               } // instance

            } // service


            struct ServiceVO
            {
               std::string name;
               std::chrono::nanoseconds timeout;
               std::string category;
               sf::platform::size::type transaction = 0;

               service::Metric metrics;
               service::Metric pending;

               sf::platform::size::type remote_invocations = 0;
               common::platform::time::point::type last;


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
                  archive & CASUAL_MAKE_NVP( category);
                  archive & CASUAL_MAKE_NVP( transaction);
                  archive & CASUAL_MAKE_NVP( metrics);
                  archive & CASUAL_MAKE_NVP( pending);
                  archive & CASUAL_MAKE_NVP( remote_invocations);
                  archive & CASUAL_MAKE_NVP( last);
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

      } // manager
   } // service
} // casual

#endif // BROKERVO_H_
