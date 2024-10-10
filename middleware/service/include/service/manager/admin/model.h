//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "casual/platform.h"

#include "common/serialize/macro.h"
#include "common/process.h"
#include "common/service/type.h"

#include "configuration/model.h"


namespace casual
{
   namespace service::manager::admin::model
   {
      inline namespace v1 { 
      
      namespace instance
      {
         struct Base
         {
            common::process::Handle process;
            std::string alias;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( process);
               CASUAL_SERIALIZE( alias);
            )

            inline friend bool operator < ( const Base& lhs, const Base& rhs) { return lhs.process < rhs.process;}
            inline friend bool operator == ( const Base& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process == rhs;}
         };

         struct Sequential : Base
         {
            enum class State : short
            {
               idle,
               busy,
            };

            constexpr std::string_view description( State value) noexcept
            {
               switch( value)
               {
                  case State::idle: return "idle";
                  case State::busy: return "busy";
               }
               return "<unknown>";
            }

            State state = State::busy;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Base::serialize( archive);
               CASUAL_SERIALIZE( state);
            )
         };

         struct Concurrent : Base
         {
            std::string description;

            CASUAL_CONST_CORRECT_SERIALIZE(
               Base::serialize( archive);
               CASUAL_SERIALIZE( description);
            )
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
               CASUAL_SERIALIZE( min);
               CASUAL_SERIALIZE( max);
            )
         } limit;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( count);
            CASUAL_SERIALIZE( total);
            CASUAL_SERIALIZE( limit);
         )
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
               CASUAL_SERIALIZE( invoked);
               CASUAL_SERIALIZE( pending);
               CASUAL_SERIALIZE( last);
               CASUAL_SERIALIZE( remote);
            )
         };

         namespace instance
         {
            struct Sequential
            {
               common::process::Handle process;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( process);
               )
            };

            struct Concurrent
            {
               common::process::Handle process;
               platform::size::type hops{};
               platform::size::type order{};

               inline friend bool operator == ( const Concurrent& lhs, common::process::compare_equal_to_handle auto rhs) { return lhs.process == rhs;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( hops);
                  CASUAL_SERIALIZE( order);
               )
            };

         } // instance

         //! @deprecated remove in 2.0 and only use Timeout in service
         struct Execution
         {
            configuration::model::service::Timeout timeout;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( timeout);
            )

         };

      } // service

      struct Service
      {
         std::string name;
         service::Execution execution;
         std::string category;
         common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
         common::service::visibility::Type visibility = common::service::visibility::Type::discoverable;

         service::Metric metric;

         struct
         {
            std::vector< service::instance::Sequential> sequential;
            std::vector< service::instance::Concurrent> concurrent;

            inline platform::size::type size() const noexcept { return sequential.size() + concurrent.size();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( sequential);
               CASUAL_SERIALIZE( concurrent);
            )
         } instances;

         inline friend bool operator == ( const Service& lhs, std::string_view rhs) { return lhs.name == rhs;}
         inline friend auto operator <=> ( const Service& lhs, const Service& rhs) { return lhs.name <=> rhs.name;}

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( execution);
            CASUAL_SERIALIZE( category);
            CASUAL_SERIALIZE( transaction);
            CASUAL_SERIALIZE( visibility);
            CASUAL_SERIALIZE( metric);
            CASUAL_SERIALIZE( instances);
            CASUAL_SERIALIZE_NAME( execution.timeout, "timeout");
         )
      };

      struct Pending
      {
         std::string requested;
         common::process::Handle process;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( requested);
            CASUAL_SERIALIZE( process);
         )
      };

      struct Route 
      {
         //! the exposed service name
         std::string service;
         //! the actual invoked service
         std::string target;

         inline friend bool operator == ( const Route& lhs, const Route& rhs) = default;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( service);
            CASUAL_SERIALIZE( target);
         )
      };

      struct Reservation
      {
         std::string service;
         common::process::Handle caller;
         common::process::Handle callee;
         common::strong::correlation::id correlation;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( service);
            CASUAL_SERIALIZE( caller);
            CASUAL_SERIALIZE( callee);
            CASUAL_SERIALIZE( correlation);
         )
      };

      struct State
      {
         struct
         {
            std::vector< instance::Sequential> sequential;
            std::vector< instance::Concurrent> concurrent;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( sequential);
               CASUAL_SERIALIZE( concurrent);
            )

         } instances;


         std::vector< Service> services;
         std::vector< Pending> pending;
         std::vector< Route> routes;
         std::vector< Reservation> reservations;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( instances);
            CASUAL_SERIALIZE( services);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( routes);
            CASUAL_SERIALIZE( reservations);
         )
      };
   
      } // inline namespace v1

   } // service::manager::admin::model
} // casual


