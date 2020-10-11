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

            enum class Type : short
            {
               sequential = 1,
               concurrent = 2,
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
               service::Type type = service::Type::sequential;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( name);
                  CASUAL_SERIALIZE( category);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( type);
               })
            };
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
               using Service = message::Service;
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
                  }
                  return out << "unknown";
               }
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

               struct
               {
                  std::vector< advertise::Service> add;
                  std::vector< std::string> remove;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( add);
                     CASUAL_SERIALIZE( remove);
                  })
               } services;


               //! @return true ii the intention is to remove all advertised services for the server
               inline bool clear() const { return services.add.empty() && services.remove.empty();}

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_advertise::serialize( archive);
                  CASUAL_SERIALIZE( services);
               })
            };

            namespace concurrent
            {
               namespace advertise
               {
                  namespace service
                  {

                     struct Property : common::Compare< Property>
                     {

                        enum class Type : short
                        {
                           configured,
                           discovered,
                        };

                        inline friend std::ostream& operator << ( std::ostream& out, Type value)
                        {
                           switch( value)
                           {
                              case Type::configured: return out << "configured";
                              case Type::discovered: return out << "discovered";
                           }
                           return out << "<unknown>";
                        }

                        platform::size::type hops = 0;
                        Type type = Type::discovered;

                        inline auto tie() const { return std::tie( type, hops);}

                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( hops);
                           CASUAL_SERIALIZE( type);
                        })
                     };


                  } // service

                  //! Represent service information in a 'advertise context'
                  struct Service : message::Service
                  {
                     Service() = default;
                     inline Service( std::string name, 
                        std::string category, 
                        common::service::transaction::Type transaction,
                        service::Property property = service::Property{})
                        : message::Service( std::move( name), std::move( category), transaction), property( property) {}

                     service::Property property;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        message::Service::serialize( archive);
                        CASUAL_SERIALIZE( property);
                     })
                  };

               } // advertise

               using basic_advertise = basic_request< message::Type::service_concurrent_advertise>;
               struct Advertise : basic_advertise
               {
                  using basic_advertise::basic_advertise;

                  platform::size::type order = 0;

                  struct
                  {
                     std::vector< advertise::Service> add;
                     std::vector< std::string> remove;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( add);
                        CASUAL_SERIALIZE( remove);
                     })
                  } services;

                  //! indicate to remove all current advertised services, and replace with content in this message
                  bool reset = false;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     basic_advertise::serialize( archive);
                     CASUAL_SERIALIZE( order);
                     CASUAL_SERIALIZE( services);
                     CASUAL_SERIALIZE( reset);
                  })
               };
              
            } // concurrent   


            namespace lookup
            {
               //! Represent "service-name-lookup" request.
               using base_request = basic_request< message::Type::service_name_lookup_request>; 
               struct Request : base_request
               {
                  using base_request::base_request;

                  enum class Context : short
                  {
                     regular,
                     forward,
                     no_busy_intermediate,
                     wait,
                  };

                  inline friend std::ostream& operator << ( std::ostream& out, const Request::Context& value)
                  {
                     switch( value)
                     {
                        case Request::Context::regular: return out << "regular";
                        case Request::Context::forward: return out << "forward";
                        case Request::Context::no_busy_intermediate: return out << "no-busy-intermediate";
                        case Request::Context::wait: return out << "wait";
                        
                     }
                     return out << "unknown";
                  }

                  std::string requested;
                  Context context = Context::regular;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( requested);
                     CASUAL_SERIALIZE( context);
                  })
               };

               //! Represent "service-name-lookup" response.
               using base_reply = basic_reply< message::Type::service_name_lookup_reply>; 
               struct Reply : base_reply
               {
                  using base_reply::base_reply;

                  call::Service service;

                  //! represent how long this request was pending (busy);
                  platform::time::unit pending{};

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
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( pending);
                     CASUAL_SERIALIZE( state);
                  })
               };

               namespace discard
               {
                  using base_request = basic_request< message::Type::service_name_lookup_discard_request>;
                  struct Request : base_request
                  {
                     using base_request::base_request;

                     std::string requested;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_request::serialize( archive);
                        CASUAL_SERIALIZE( requested);
                     })
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
                        base_reply::serialize( archive);
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

               template< message::Type type>
               struct common_request : message::basic_request< type>
               {
                  using base_type = message::basic_request< type>;
                  using base_type::base_type;

                  Service service;
                  std::string parent;

                  common::transaction::ID trid;
                  request::Flags flags;

                  common::service::header::Fields header;

                  //! pending time, only to be return in the "ACK", to collect
                  //! metrics
                  platform::time::unit pending{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_type::serialize( archive);
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( parent);
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( flags);
                     CASUAL_SERIALIZE( header);
                     CASUAL_SERIALIZE( pending);
                  })
               };

               using base_request = common_request< message::Type::service_call>;
               struct basic_request : base_request
               {
                  using base_request::base_request;
                  
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
               } // caller

               namespace callee
               {
                  //! Represents a service call. via tp(a)call, from the callee's perspective
                  using Request = message::buffer::callee::basic_request< call::basic_request>;
               } // callee


               //! Represent service reply.
               using base_reply = basic_message< message::Type::service_reply>;
               struct Reply : base_reply
               {
                  service::Code code;
                  Transaction transaction;
                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( code);
                     CASUAL_SERIALIZE( transaction);
                     CASUAL_SERIALIZE( buffer);
                  })
               };

               //! Represent the reply to the service-manager when a server is done handling
               //! a service-call and is ready for new calls
               using base_ack = basic_message< message::Type::service_acknowledge>;
               struct ACK : base_ack
               {
                  event::service::Metric metric;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_ack::serialize( archive);
                     CASUAL_SERIALIZE( metric);
                  })
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
            struct type_traits< service::call::caller::Request> : detail::type<  service::call::Reply> {};

            template<>
            struct type_traits< service::call::callee::Request> : detail::type<  service::call::Reply> {};

         } // reverse

      } // message

   } // common


} // casual


