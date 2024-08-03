//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/message/type.h"
#include "common/message/event.h"

#include "common/transaction/id.h"
#include "common/service/type.h"
#include "common/buffer/type.h"
#include "common/uuid.h"
#include "common/flag/xatmi.h"
#include "common/code/xatmi.h"
#include "common/algorithm/compare.h"

#include "common/service/header.h"

#include "common/serialize/line.h"

namespace casual
{
   namespace common::message
   {
      namespace service
      {
         struct Code 
         {
            code::xatmi result = code::xatmi::ok;
            long user{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( result);
               CASUAL_SERIALIZE( user);
            )
         };

         enum class Type : short
         {
            sequential = 1,
            concurrent = 2,
         };

         struct Base : common::Compare < Base>
         {
            Base() = default;

            explicit Base( std::string name, std::string category, common::service::transaction::Type transaction, common::service::visibility::Type visibility)
               : name( std::move( name)), category( std::move( category)), transaction( transaction), visibility{ visibility}
            {}

            explicit Base( std::string name)
               : name( std::move( name))
            {}

            std::string name;
            std::string category;
            common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
            common::service::visibility::Type visibility = common::service::visibility::Type::discoverable;
            service::Type type = service::Type::sequential;

            inline friend bool operator == ( const Base& lhs, std::string_view rhs) { return lhs.name == rhs;}

            inline auto tie() const noexcept { return std::tie( name);}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( category);
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( visibility);
               CASUAL_SERIALIZE( type);
            )
         };

         struct Timeout
         {
            platform::time::unit duration{};

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( duration);
            )

         };

      } // service

      struct Service : service::Base
      {
         using service::Base::Base;

         service::Timeout timeout;

         CASUAL_CONST_CORRECT_SERIALIZE(
            service::Base::serialize( archive);
            CASUAL_SERIALIZE( timeout);
         )
      };

      namespace service
      {
         namespace call
         {
            //! Represent service information in a 'call context'
            struct Service : message::Service
            {
               using message::Service::Service;

               // if the requested service name differs from the 'origin', requested is set.
               std::optional< std::string> requested;

               inline auto logical_name() const noexcept { return requested.value_or( name); }

               CASUAL_CONST_CORRECT_SERIALIZE(
                  message::Service::serialize( archive);
                  CASUAL_SERIALIZE( requested);
               )
            };
         } // call

         namespace transaction
         {
            enum class State : std::uint8_t
            {
               ok,
               rollback,
               timeout,
               error,
            };

            //! 'accumulate' State, only more severe
            //! @{
            inline State operator + ( State lhs, State rhs) noexcept { return std::max( lhs, rhs);}
            inline State& operator += ( State& lhs, State rhs) noexcept { return lhs = lhs + rhs;}   
            //! @}

            inline constexpr std::string_view description( State value) noexcept
            {
               switch( value)
               {
                  case State::ok: return "ok";
                  case State::rollback: return "rollback";
                  case State::timeout: return "timeout";
                  case State::error: return "error";
               }
               return "<unknown>";
            }

         } // transaction

         struct Transaction
         {
            common::transaction::ID trid;
            transaction::State state = transaction::State::ok;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( trid);
               CASUAL_SERIALIZE( state);
            )
         };

         namespace advertise
         {
            //! Represent service information in a 'advertise context'
            using Service = message::service::Base;

         } // advertise

         using base_advertise = basic_request< message::Type::service_advertise>;
         struct Advertise : base_advertise
         {
            using base_advertise::base_advertise;

            //! the alias of the server 'instance'
            std::string alias;

            struct
            {
               std::vector< advertise::Service> add;
               std::vector< std::string> remove;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( add);
                  CASUAL_SERIALIZE( remove);
               )
            } services;


            //! @return true ii the intention is to remove all advertised services for the server
            inline bool clear() const { return services.add.empty() && services.remove.empty();}

            CASUAL_CONST_CORRECT_SERIALIZE(
               base_advertise::serialize( archive);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( services);
            )
         };

         namespace concurrent
         {
            namespace advertise
            {
               namespace service
               {
                  namespace property
                  {
                     enum class Type : short
                     {
                        configured,
                        discovered,
                     };

                     constexpr std::string_view description( Type value) noexcept
                     {
                        switch( value)
                        {
                           case Type::configured: return "configured";
                           case Type::discovered: return "discovered";
                        }
                        return "<unknown>";
                     }
                  } // property

                  struct Property
                  {
                     property::Type type = property::Type::discovered;
                     platform::size::type hops = 0;
                     
                     friend auto operator <=> ( const Property&, const Property&) = default;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( type);
                        CASUAL_SERIALIZE( hops);
                     )
                  };

               } // service

               //! Represent service information in a 'advertise context'
               struct Service : message::Service
               {
                  Service() = default;
                  inline Service( std::string name, 
                     std::string category, 
                     common::service::transaction::Type transaction,
                     common::service::visibility::Type visibility,
                     service::Property property = service::Property{})
                     : message::Service( std::move( name), std::move( category), transaction, visibility), property( property) {}

                  service::Property property;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     message::Service::serialize( archive);
                     CASUAL_SERIALIZE( property);
                  )
               };

               enum struct Directive
               {
                  update,   //! regular update
                  reset,    //! remove all previous associated services for the 'device'
                  instance, //! instance (order) only 
               };

               constexpr std::string_view description( Directive value) noexcept
               {
                  switch( value)
                  {
                     case Directive::update: return "update";
                     case Directive::reset: return "reset";
                     case Directive::instance: return "instance";
                  }
                  return "<unknown>";
               }

            } // advertise

            using basic_advertise = basic_request< message::Type::service_concurrent_advertise>;
            struct Advertise : basic_advertise
            {
               using basic_advertise::basic_advertise;

               //! the alias of the server 'instance'
               std::string alias;
               //! A human readable description of the "instance"
               std::string description;
               platform::size::type order = 0;
               advertise::Directive directive{};

               struct
               {
                  std::vector< advertise::Service> add;
                  std::vector< std::string> remove;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( add);
                     CASUAL_SERIALIZE( remove);
                  )
               } services;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  basic_advertise::serialize( archive);
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( description);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( directive);
                  CASUAL_SERIALIZE( services);
               )
            };
            
         } // concurrent   


         namespace lookup
         {
            namespace request
            {
               namespace context
               {
                  enum class Semantic : short
                  {
                     regular,
                     no_reply,
                     no_busy_intermediate,
                     wait,
                     // used fromm service-forward only (fire-and-forget)
                     forward_request,
                  };

                  inline constexpr std::string_view description( Semantic value) noexcept
                  {
                     switch( value)
                     {
                        case Semantic::regular: return "regular";
                        case Semantic::no_reply: return "no_reply";
                        case Semantic::no_busy_intermediate: return "no_busy_intermediate";
                        case Semantic::wait: return "wait";
                        case Semantic::forward_request: return "forward_request";
                     }
                     return "<unknown>";
                  }

                  enum class Requester : short
                  {
                     internal,
                     external,
                     // connection configured with discovery.forward 
                     external_discovery,
                  };
                  
                  inline constexpr std::string_view description( Requester value) noexcept
                  {
                     switch( value)
                     {
                        case Requester::internal: return "internal";
                        case Requester::external: return "external"; 
                        case Requester::external_discovery: return "external_discovery";                     
                     }
                     return "<unknown>";
                  }
               } // context

               struct Context
               {
                  Context() = default;
                  inline Context( context::Semantic semantic, context::Requester requester) 
                     : semantic{ semantic}, requester{ requester} {}

                  inline Context( context::Semantic semantic) : Context( semantic, context::Requester{}) {}

                  context::Semantic semantic{};
                  context::Requester requester{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( semantic);
                  CASUAL_SERIALIZE( requester);
               )

               };
            } // request

            //! Represent "service-name-lookup" request.
            using base_request = basic_request< message::Type::service_name_lookup_request>; 
            struct Request : base_request
            {
               using base_request::base_request;

               std::string requested;
               request::Context context;
               common::transaction::global::ID gtrid;
               std::optional< platform::time::point::type> deadline{};

               inline bool no_reply() const noexcept
               {
                  using Enum = request::context::Semantic;
                  return algorithm::compare::any( context.semantic, Enum::no_reply, Enum::forward_request);
               }

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( requested);
                  CASUAL_SERIALIZE( context);
                  CASUAL_SERIALIZE( gtrid);
                  CASUAL_SERIALIZE( deadline);
               )
            };

            namespace reply
            {
               enum class State : short
               {
                  absent,
                  busy,
                  idle
               };

               inline constexpr std::string_view description( State value) noexcept
               {
                  switch( value)
                  {
                     case State::absent: return "absent";
                     case State::idle: return "idle";
                     case State::busy: return "busy";
                  }
                  return "<unknown>";
               }
            } // reply

            //! Represent "service-name-lookup" response.
            using base_reply = basic_process< message::Type::service_name_lookup_reply>; 
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               call::Service service;
               //! represent how long this request was pending (busy);
               platform::time::unit pending{};
               reply::State state = reply::State::idle;
               
               inline bool busy() const { return state == reply::State::busy;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( state);
               )
            };

            namespace discard
            {
               using base_request = basic_request< message::Type::service_name_lookup_discard_request>;
               struct Request : base_request
               {
                  using base_request::base_request;

                  std::string requested;
                  //! if caller want's a reply or not, default: true
                  bool reply = true;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( requested);
                     CASUAL_SERIALIZE( reply);
                  )
               };

               using base_reply = basic_message< message::Type::service_name_lookup_discard_reply>;
               struct Reply : base_reply
               {
                  enum class State : short
                  {
                     absent,
                     discarded,
                     replied
                  };

                  constexpr std::string_view description( State value) noexcept
                  {
                     switch( value)
                     {
                        case Reply::State::absent: return "absent";
                        case Reply::State::discarded: return "discarded";
                        case Reply::State::replied: return "replied";
                     }
                     return "<unknown>";
                  }

                  State state = State::absent;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( state);
                  )
               };
            } // discard
         } // lookup


         namespace call
         {
            namespace request
            {
               enum class Flag : long
               {
                  no_transaction = std::to_underlying( flag::xatmi::Flag::no_transaction),
                  no_reply = std::to_underlying( flag::xatmi::Flag::no_reply),
                  no_time = std::to_underlying( flag::xatmi::Flag::no_time),
               };
               
               // indicate that this enum is used as a flag
               consteval flag::xatmi::Flag casual_enum_as_flag_superset( Flag);

            } // request

            namespace v1_2
            {
               struct base_request : message::basic_request< message::Type::service_call_v2>
               {
                  using base_type = message::basic_request< message::Type::service_call_v2>;
                  using base_type::base_type;

                  Service service;
                  std::string parent;

                  common::transaction::ID trid;
                  request::Flag flags{};

                  common::service::header::Fields header;

                  //! pending time, only to be return in the "ACK", to collect
                  //! metrics
                  platform::time::unit pending{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( parent);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( flags);
                     CASUAL_SERIALIZE( header);
                     CASUAL_SERIALIZE( pending);
                  )
               };

               namespace caller
               {
                  //! Represents a service call. via tp(a)call, from the callers perspective
                  struct Request : base_request
                  {
                     template< typename... Args>
                     Request( common::buffer::payload::Send buffer, Args&&... args)
                        : base_request( std::forward< Args>( args)...), buffer( std::move( buffer))
                     {}
                     
                     common::buffer::payload::Send buffer;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( buffer);
                     )
                  };

               } // caller

               namespace callee
               {
                  //! Represents a service call. via tp(a)call, from the callee's perspective
                  struct Request : base_request
                  {   
                     using base_request::base_request;

                     common::buffer::Payload buffer;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( buffer);
                     )
                  };
               } // callee


               //! Represent service reply.
               using base_reply = basic_message< message::Type::service_reply_v2>;
               struct Reply : base_reply
               {
                  service::Code code;
                  Transaction transaction;
                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( code);
                     CASUAL_SERIALIZE( transaction);
                     CASUAL_SERIALIZE( buffer);
                  )
               };
            } // v1_2


            struct base_request : message::basic_request< message::Type::service_call>
            {
               using base_type = message::basic_request< message::Type::service_call>;
               using base_type::base_type;

               execution::context::Parent parent;
               Service service;

               common::transaction::ID trid;
               request::Flag flags{};

               common::service::header::Fields header;

               //! pending time, only to be return in the "ACK", to collect
               //! metrics
               platform::time::unit pending{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_type::serialize( archive);
                  CASUAL_SERIALIZE( parent);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( flags);
                  CASUAL_SERIALIZE( header);
                  CASUAL_SERIALIZE( pending);
               )
            };

            namespace caller
            {
               //! Represents a service call. via tp(a)call, from the callers perspective
               struct Request : base_request
               {
                  template< typename... Args>
                  Request( common::buffer::payload::Send buffer, Args&&... args)
                     : base_request( std::forward< Args>( args)...), buffer( std::move( buffer))
                  {}
                  
                  common::buffer::payload::Send buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( buffer);
                  )
               };

            } // caller

            namespace callee
            {
               //! Represents a service call. via tp(a)call, from the callee's perspective
               struct Request : base_request
               {   
                  using base_request::base_request;

                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( buffer);
                  )
               };
            } // callee


            //! Represent service reply.
            using base_reply = basic_message< message::Type::service_reply>;
            struct Reply : base_reply
            {
               service::Code code;
               transaction::State transaction_state = transaction::State::ok;
               common::buffer::Payload buffer;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( code);
                  CASUAL_SERIALIZE( transaction_state);
                  CASUAL_SERIALIZE( buffer);
               )
            };

            //! Represent the reply to the service-manager when a server is done handling
            //! a service-call and is ready for new calls
            using base_ack = basic_message< message::Type::service_acknowledge>;
            struct ACK : base_ack
            {
               event::service::Metric metric;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_ack::serialize( archive);
                  CASUAL_SERIALIZE( metric);
               )
            };
            
         } // call

      } // service

      namespace reverse
      {

         template<>
         struct type_traits< service::lookup::Request> : detail::type< service::lookup::Reply> {};

         template<>
         struct type_traits< service::lookup::discard::Request> : detail::type< service::lookup::discard::Reply> {};

         template<>
         struct type_traits< service::call::callee::Request> : detail::type<  service::call::Reply> {};

         template<>
         struct type_traits< service::call::caller::Request> : detail::type<  service::call::Reply> {};

         //template<>
         //struct type_traits< service::call::v1_2::caller::Request> : detail::type<  service::call::v1_2::Reply> {};

         template<>
         struct type_traits< service::call::v1_2::callee::Request> : detail::type<  service::call::v1_2::Reply> {};

      } // reverse

   } // common::message

} // casual


