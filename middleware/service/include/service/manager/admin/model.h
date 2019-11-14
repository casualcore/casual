//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/process.h"


namespace casual
{
   namespace service
   {
      namespace manager
      {
         namespace admin
         {
            namespace model
            {
               inline namespace v1 { 
               
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

                  struct Sequential : Base
                  {
                     enum class State : short
                     {
                        idle,
                        busy,
                     };

                     State state = State::busy;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        Base::serialize( archive);
                        CASUAL_SERIALIZE( state);
                     })
                  };

                  struct Concurrent : Base
                  {
                  };

               } // instance

               struct Metric
               {
                  platform::size::type count = 0;
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

               namespace service
               {
                  struct Metric
                  {
                     model::Metric invoked;
                     model::Metric pending;

                     platform::time::point::type last = platform::time::point::limit::zero();
                     platform::size::type remote = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( invoked);
                        CASUAL_SERIALIZE( pending);
                        CASUAL_SERIALIZE( last);
                        CASUAL_SERIALIZE( remote);
                     })
                  };

                  namespace instance
                  {
                     struct Sequential
                     {
                        common::strong::process::id pid;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( pid);
                        })
                     };

                     struct Concurrent
                     {
                        common::strong::process::id pid;
                        platform::size::type hops;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( pid);
                           CASUAL_SERIALIZE( hops);
                        })
                     };

                  } // instance

               } // service

               struct Service
               {
                  enum class Transaction : platform::size::type
                  {
                     automatic = 0,
                     join = 1,
                     atomic = 2,
                     none = 3,
                     branch,
                  };

                  std::string name;
                  std::chrono::nanoseconds timeout;
                  std::string category;
                  Transaction transaction = Transaction::automatic;

                  service::Metric metric;

                  struct
                  {
                     std::vector< service::instance::Sequential> sequential;
                     std::vector< service::instance::Concurrent> concurrent;

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
                     CASUAL_SERIALIZE( metric);
                     CASUAL_SERIALIZE( instances);
                  })
               };

               struct Pending
               {
                  std::string requested;
                  common::process::Handle process;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( requested);
                     CASUAL_SERIALIZE( process);
                  })
               };

               struct Route 
               {
                  //! the exposed service name
                  std::string service;
                  //! the actual invoked service
                  std::string target;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( target);
                  })
               };

               struct State
               {
                  struct
                  {
                     std::vector< instance::Sequential> sequential;
                     std::vector< instance::Concurrent> concurrent;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( sequential);
                        CASUAL_SERIALIZE( concurrent);
                     })

                  } instances;


                  std::vector< Service> services;
                  std::vector< Pending> pending;
                  std::vector< Route> routes;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( instances);
                     CASUAL_SERIALIZE( services);
                     CASUAL_SERIALIZE( pending);
                     CASUAL_SERIALIZE( routes);
                  })

               };
            
               } // inline namespace v1
            } // model
         } // admin
      } // manager
   } // service
} // casual


