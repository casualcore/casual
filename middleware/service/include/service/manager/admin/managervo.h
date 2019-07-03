//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "common/platform.h"
#include "common/process.h"


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
                     CASUAL_SERIALIZE( process);
                  })

                  friend bool operator < ( const Base& lhs, const Base& rhs) { return lhs.process.pid < rhs.process.pid;}
                  friend bool operator == ( const Base& lhs, const Base& rhs) { return lhs.process.pid == rhs.process.pid;}
                  friend bool operator == ( const Base& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}

               };

               struct SequentialVO : Base
               {

                  enum class State : short
                  {
                     idle,
                     busy,
                     exiting,
                  };

                  State state = State::busy;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     Base::serialize( archive);
                     CASUAL_SERIALIZE( state);
                  })
               };

               struct ConcurrentVO : Base
               {
               };

            } // instance


            namespace service
            {
               struct Metric
               {
                  common::platform::size::type count = 0;
                  std::chrono::nanoseconds total = std::chrono::nanoseconds::zero();

                  struct Limit 
                  {
                     std::chrono::nanoseconds min = std::chrono::nanoseconds::zero();
                     std::chrono::nanoseconds max = std::chrono::nanoseconds::zero();

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( min);
                        CASUAL_SERIALIZE( max);
                     })
                  } limit;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( count);
                     CASUAL_SERIALIZE( total);
                     CASUAL_SERIALIZE( limit);
                  })
               };

               namespace instance
               {

                  struct Local
                  {
                     common::strong::process::id pid;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( pid);
                     })
                  };

                  struct Remote
                  {
                     common::strong::process::id pid;
                     common::platform::size::type hops;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( pid);
                        CASUAL_SERIALIZE( hops);
                     })
                  };

               } // instance

            } // service


            struct ServiceVO
            {
               std::string name;
               std::chrono::nanoseconds timeout;
               std::string category;
               common::platform::size::type transaction = 0;

               service::Metric metrics;
               service::Metric pending;

               common::platform::size::type remote_invocations = 0;
               common::platform::time::point::type last = common::platform::time::point::limit::zero();


               struct
               {
                  std::vector< service::instance::Local> sequential;
                  std::vector< service::instance::Remote> concurrent;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( sequential);
                     CASUAL_SERIALIZE( concurrent);
                  })
               } instances;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( timeout);
                  CASUAL_SERIALIZE( category);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( metrics);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( remote_invocations);
                  CASUAL_SERIALIZE( last);
                  CASUAL_SERIALIZE( instances);
               })
            };

            struct PendingVO
            {
               std::string requested;
               common::process::Handle process;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( requested);
                  CASUAL_SERIALIZE( process);
               })
            };

            struct StateVO
            {
               struct
               {
                  std::vector< instance::SequentialVO> sequential;
                  std::vector< instance::ConcurrentVO> concurrent;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( sequential);
                     CASUAL_SERIALIZE( concurrent);
                  })

               } instances;


               std::vector< ServiceVO> services;
               std::vector< PendingVO> pending;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( pending);
               })

            };

         } // admin

      } // manager
   } // service
} // casual


