//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"


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
                     archive & CASUAL_MAKE_NVP( state);
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
                  serviceframework::platform::size::type count = 0;
                  std::chrono::nanoseconds total = std::chrono::nanoseconds::zero();

                  struct Limit 
                  {
                     std::chrono::nanoseconds min = std::chrono::nanoseconds::zero();
                     std::chrono::nanoseconds max = std::chrono::nanoseconds::zero();

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( min);
                        archive & CASUAL_MAKE_NVP( max);
                     })
                  } limit;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( count);
                     archive & CASUAL_MAKE_NVP( total);
                     archive & CASUAL_MAKE_NVP( limit);
                  })
               };

               namespace instance
               {

                  struct Local
                  {
                     serviceframework::strong::process::id pid;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        archive & CASUAL_MAKE_NVP( pid);
                     })
                  };

                  struct Remote
                  {
                     serviceframework::strong::process::id pid;
                     serviceframework::platform::size::type hops;

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
               serviceframework::platform::size::type transaction = 0;

               service::Metric metrics;
               service::Metric pending;

               serviceframework::platform::size::type remote_invocations = 0;
               common::platform::time::point::type last;


               struct
               {
                  std::vector< service::instance::Local> sequential;
                  std::vector< service::instance::Remote> concurrent;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( sequential);
                     archive & CASUAL_MAKE_NVP( concurrent);
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
                  std::vector< instance::SequentialVO> sequential;
                  std::vector< instance::ConcurrentVO> concurrent;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     archive & CASUAL_MAKE_NVP( sequential);
                     archive & CASUAL_MAKE_NVP( concurrent);
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


