//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/type.h"
#include "common/message/buffer.h"
#include "common/message/event.h"

#include "common/transaction/id.h"
#include "common/service/type.h"
#include "common/buffer/type.h"
#include "common/uuid.h"
#include "common/flag/xatmi.h"
#include "common/code/xatmi.h"

#include "common/service/header.h"

#include "common/serialize/line.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace service
         {

            struct Code 
            {
               code::xatmi result = code::xatmi::ok;
               long user = 0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( result);
                  CASUAL_SERIALIZE( user);
               })
            };

            struct Base
            {
               Base() = default;

               explicit Base( std::string name, std::string category, common::service::transaction::Type transaction)
                  : name( std::move( name)), category( std::move( category)), transaction( transaction)
               {}

               Base( std::string name)
                  : name( std::move( name))
               {}

               std::string name;
               std::string category;
               common::service::transaction::Type transaction = common::service::transaction::Type::automatic;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( category);
                  CASUAL_SERIALIZE( transaction);
               })
            };
            static_assert( traits::is_movable< Base>::value, "not movable");

         } // service

         struct Service : service::Base
         {
            using service::Base::Base;

            platform::time::unit timeout = platform::time::unit::zero();

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               service::Base::serialize( archive);
               CASUAL_SERIALIZE( timeout);
            })
         };

         namespace service
         {
            namespace call
            {
               //! Represent service information in a 'call context'
               struct Service : message::Service
               {
                  using message::Service::Service;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     message::Service::serialize( archive);
                  })
               };
            } // call

            struct Transaction
            {
               // TODO: major-version change to short 
               enum class State : char
               {
                  absent,
                  active = absent,
                  rollback,
                  timeout,
                  error,
               };

               common::transaction::ID trid;
               State state = State::active;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( state);
               })

               inline friend std::ostream& operator << ( std::ostream& out, Transaction::State value)
               {
                  switch( value)
                  {
                     case Transaction::State::error: return out << "error";
                     case Transaction::State::active: return out << "active";
                     case Transaction::State::rollback: return out << "rollback";
                     case Transaction::State::timeout: return out << "timeout";
                     default: return out << "unknown";
                  }
               }
            };
            static_assert( traits::is_movable< Transaction>::value, "not movable");

            namespace advertise
            {
               //! Represent service information in a 'advertise context'
               using Service = message::service::Base;

               static_assert( traits::is_movable< Service>::value, "not movable");

            } // advertise


            struct Advertise : basic_message< Type::service_advertise>
            {
               enum class Directive : short
               {
                  add,
                  remove
               };

               inline friend std::ostream& operator << ( std::ostream& out, Advertise::Directive value)
               {
                  switch( value)
                  {
                     case Advertise::Directive::add: return out << "add";
                     case Advertise::Directive::remove: return out << "remove";
                  }
                  return out << "unknown";
               }

               Directive directive = Directive::add;

               common::process::Handle process;
               std::vector< advertise::Service> services;


               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_type::serialize( archive);
                  CASUAL_SERIALIZE( directive);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( services);
               })
            };
            static_assert( traits::is_movable< Advertise>::value, "not movable");

            namespace concurrent
            {
               namespace advertise
               {
                  //! Represent service information in a 'advertise context'
                  struct Service : message::Service
                  {
                     Service() = default;
                     inline Service( std::string name, 
                        std::string category, 
                        common::service::transaction::Type transaction,
                        platform::size::type hops = 0)
                        : message::Service( std::move( name), std::move( category), transaction), hops( hops) {}

                     platform::size::type hops = 0;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        message::Service::serialize( archive);
                        CASUAL_SERIALIZE( hops);
                     })
                  };

                  static_assert( traits::is_movable< Service>::value, "not movable");

               } // advertise


               struct Advertise : basic_message< Type::service_concurrent_advertise>
               {
                  enum class Directive : short
                  {
                     add,
                     remove
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, Advertise::Directive value)
                  {
                     switch( value)
                     {
                        case Advertise::Directive::add: return out << "add";
                        case Advertise::Directive::remove: return out << "remove";
                     }
                     return out << "unknown";
                  }

                  Directive directive = Directive::add;

                  common::process::Handle process;
                  platform::size::type order = 0;
                  std::vector< advertise::Service> services;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( directive);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( order);
                     CASUAL_SERIALIZE( services);
                  })
               };
               static_assert( traits::is_movable< Advertise>::value, "not movable");
              
            } // concurrent   


            namespace lookup
            {
               //! Represent "service-name-lookup" request.
               struct Request : basic_message< Type::service_name_lookup_request>
               {
                  enum class Context : short
                  {
                     regular,
                     no_reply,
                     forward,
                     gateway,
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, const Request::Context& value)
                  {
                     switch( value)
                     {
                        case Request::Context::forward: return out << "forward";
                        case Request::Context::gateway: return out << "gateway";
                        case Request::Context::no_reply: return out << "no_reply";
                        case Request::Context::regular: return out << "regular";
                     }
                     return out << "unknown";
                  }

                  std::string requested;
                  common::process::Handle process;
                  Context context = Context::regular;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( requested);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( context);
                  })
               };
               static_assert( traits::is_movable< Request>::value, "not movable");


               //! Represent "service-name-lookup" response.
               struct Reply : basic_message< Type::service_name_lookup_reply>
               {
                  call::Service service;
                  common::process::Handle process;

                  enum class State : short
                  {
                     absent,
                     busy,
                     idle
                  };
                  inline friend std::ostream& operator << ( std::ostream& out, State value)
                  {
                     switch( value)
                     {
                        case Reply::State::absent: return out << "absent";
                        case Reply::State::idle: return out << "idle";
                        case Reply::State::busy: return out << "busy";
                     }
                     return out << "unknown";
                  }


                  State state = State::idle;

                  inline bool busy() const { return state == State::busy;}

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( state);
                  })
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

               namespace discard
               {
                  struct Request : basic_message< Type::service_name_lookup_discard_request>
                  {
                     std::string requested;
                     common::process::Handle process;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( requested);
                        CASUAL_SERIALIZE( process);
                     })
                  };

                  struct Reply : basic_message< Type::service_name_lookup_discard_reply>
                  {
                     enum class State : short
                     {
                        absent,
                        discarded,
                        replied
                     };
                     inline friend std::ostream& operator << ( std::ostream& out, State value)
                     {
                        switch( value)
                        {
                           case Reply::State::absent: return out << "absent";
                           case Reply::State::discarded: return out << "discarded";
                           case Reply::State::replied: return out << "replied";
                        }
                        return out << "unknown";
                     }

                     State state = State::absent;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( state);
                     })
                  };
               } // discard
            } // lookup


            namespace call
            {
               namespace request
               {
                  enum class Flag : long
                  {
                     no_transaction = cast::underlying( flag::xatmi::Flag::no_transaction),
                     no_reply = cast::underlying( flag::xatmi::Flag::no_reply),
                     no_time = cast::underlying( flag::xatmi::Flag::no_time),
                  };
                  using Flags = common::Flags< Flag>;

               } // request
               struct common_request
               {
                  common::process::Handle process;

                  Service service;
                  std::string parent;

                  common::transaction::ID trid;
                  request::Flags flags;

                  common::service::header::Fields header;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( process);
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( parent);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( flags);
                     CASUAL_SERIALIZE( header);
                  })
               };

               using base_request = type_wrapper< common_request, Type::service_call>;
               struct basic_request : base_request
               {
                  request::Flags flags;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( flags);
                  )
               };


               namespace caller
               {
                  //! Represents a service call. via tp(a)call, from the callers perspective
                  using Request = message::buffer::caller::basic_request< call::basic_request>;

                  static_assert( traits::is_movable< Request>::value, "not movable");
               } // caller

               namespace callee
               {
                  //! Represents a service call. via tp(a)call, from the callee's perspective
                  using Request = message::buffer::callee::basic_request< call::basic_request>;

                  static_assert( traits::is_movable< Request>::value, "not movable");

               } // callee


               //! Represent service reply.
               struct Reply :  basic_message< Type::service_reply>
               {
                  service::Code code;
                  Transaction transaction;
                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( code);
                     CASUAL_SERIALIZE( transaction);
                     CASUAL_SERIALIZE( buffer);
                  })
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

               //! Represent the reply to the service-manager when a server is done handling
               //! a service-call and is ready for new calls
               struct ACK : basic_message< Type::service_acknowledge>
               {
                  event::service::Metric metric;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( metric);
                  })
               };
               static_assert( traits::is_movable< ACK>::value, "not movable");

               
            } // call



         } // service

         namespace reverse
         {

            template<>
            struct type_traits< service::lookup::Request> : detail::type< service::lookup::Reply> {};

            template<>
            struct type_traits< service::lookup::discard::Request> : detail::type< service::lookup::discard::Reply> {};

            template<>
            struct type_traits< service::call::caller::Request> : detail::type<  service::call::Reply> {};

            template<>
            struct type_traits< service::call::callee::Request> : detail::type<  service::call::Reply> {};

         } // reverse

      } // message

   } // common


} // casual


