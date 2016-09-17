//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_


#include "gateway/message.h"

#include "common/message/type.h"
#include "common/message/transaction.h"
#include "common/message/service.h"
#include "common/message/gateway.h"
#include "common/domain.h"

#include <thread>



//!
//! global overload for XID
//!
//! @{
template< typename M>
void casual_marshal_value( const XID& value, M& marshler)
{
   marshler << value.formatID;

   if( ! casual::common::transaction::null( value))
   {
      marshler << value.gtrid_length;
      marshler << value.bqual_length;

      marshler.append( casual::common::transaction::data( value));
   }
}

template< typename M>
void casual_unmarshal_value( XID& value, M& unmarshler)
{
   unmarshler >> value.formatID;

   if( ! casual::common::transaction::null( value))
   {
      unmarshler >> value.gtrid_length;
      unmarshler >> value.bqual_length;

      unmarshler.consume(
         std::begin( value.data),
         value.gtrid_length + value.bqual_length);
   }

}
//! @}


namespace casual
{
   namespace gateway
   {
      namespace message
      {

         namespace manager
         {

            namespace listener
            {
               struct Event : common::message::basic_message< common::message::Type::gateway_manager_listener_event>
               {

                  enum class State
                  {
                     running,
                     exit,
                     signal,
                     error
                  };

                  State state;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                     archive & state;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Event::State& value);
                  friend std::ostream& operator << ( std::ostream& out, const Event& value);
               };

            } // listener
         } // manager


         template< common::message::Type type>
         struct basic_connect : common::message::basic_message< type>
         {
            common::process::Handle process;
            std::vector< std::string> address;

            CASUAL_CONST_CORRECT_MARSHAL({
               common::message::basic_message< type>::marshal( archive);
               archive & process;
               archive & address;
            })

            friend std::ostream& operator << ( std::ostream& out, const basic_connect& value)
            {
               return out << "{ process: " << value.process
                     << ", address: " << common::range::make( value.address)
                     << '}';
            }
         };

         namespace outbound
         {
            struct Connect : basic_connect< common::message::Type::gateway_outbound_connect>
            {
            };

         } // outbound

         namespace inbound
         {
            struct Connect : basic_connect< common::message::Type::gateway_inbound_connect>
            {
            };
         } // inbound

         namespace ipc
         {
            namespace connect
            {

               struct Request : basic_connect< common::message::Type::gateway_ipc_connect_request>
               {
                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };

               struct Reply : basic_connect< common::message::Type::gateway_ipc_connect_reply>
               {
                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };

            } // connect

         } // ipc

         namespace tcp
         {
            struct Connect : common::message::basic_message< common::message::Type::gateway_manager_tcp_connect>
            {
               common::platform::tcp::descriptor::type descriptor;

               friend std::ostream& operator << ( std::ostream& out, const Connect& value);

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & descriptor;
               })
            };

         } // tcp


         namespace worker
         {

            struct Connect : common::message::basic_message< common::message::Type::gateway_worker_connect>
            {
               common::platform::binary_type information;

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & information;
               })

            };

            struct Disconnect : common::message::basic_message< common::message::Type::gateway_worker_disconnect>
            {
               enum class Reason : char
               {
                  invalid,
                  signal,
                  disconnect
               };

               Disconnect() = default;
               Disconnect( Reason reason) : reason( reason) {}

               common::domain::Identity remote;
               Reason reason = Reason::invalid;

               friend std::ostream& operator << ( std::ostream& out, Reason value);
               friend std::ostream& operator << ( std::ostream& out, const Disconnect& value);

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & remote;
                  archive & reason;
               })

            };


         } // worker

         //!
         //! Wrappers for interdomain communications
         //!
         namespace interdomain
         {
            namespace detail
            {
               //!
               //! constexpr functions to help map internal message types against interdomain message types
               //!
               //!
               constexpr common::message::Type message_type( common::message::transaction::resource::prepare::Request&&)
               { return common::message::Type::ineterdomain_transaction_resource_prepare_request;}

               constexpr common::message::Type message_type( common::message::transaction::resource::prepare::Reply&&)
               { return common::message::Type::ineterdomain_transaction_resource_prepare_reply;}

               constexpr common::message::Type message_type( common::message::transaction::resource::commit::Request&&)
               { return common::message::Type::ineterdomain_transaction_resource_commit_request;}

               constexpr common::message::Type message_type( common::message::transaction::resource::commit::Reply&&)
               { return common::message::Type::ineterdomain_transaction_resource_commit_reply;}

               constexpr common::message::Type message_type( common::message::transaction::resource::rollback::Request&&)
               { return common::message::Type::ineterdomain_transaction_resource_rollback_request;}

               constexpr common::message::Type message_type( common::message::transaction::resource::rollback::Reply&&)
               { return common::message::Type::ineterdomain_transaction_resource_rollback_reply;}

               constexpr common::message::Type message_type( common::message::service::call::callee::Request&&)
               { return common::message::Type::ineterdomain_service_call;}

               constexpr common::message::Type message_type( common::message::service::call::Reply&&)
               { return common::message::Type::ineterdomain_service_reply;}

               constexpr common::message::Type message_type( common::message::gateway::domain::discover::internal::Request&&)
               { return common::message::Type::ineterdomain_domain_discover_request;}

               constexpr common::message::Type message_type( common::message::gateway::domain::discover::internal::Reply&&)
               { return common::message::Type::ineterdomain_domain_discover_reply;}



               template< typename Message>
               struct external_send_wrapper : std::reference_wrapper< Message>
               {
                  using concrete_message = Message;
                  using base_type = std::reference_wrapper< concrete_message>;

                  external_send_wrapper( Message& message)
                     : base_type{ message}, correlation( message.correlation), execution( message.execution)  {}

                  constexpr static common::message::Type type() { return message_type( Message{});}

                  common::Uuid& correlation;
                  common::Uuid& execution;

               };

               template< typename Message>
               struct external_receive_wrapper : Message
               {
                  using concrete_message = Message;
                  using concrete_message::concrete_message;

                  constexpr static common::message::Type type() { return message_type( Message{});}

                  concrete_message& get() { return static_cast< concrete_message&>( *this);}
                  const concrete_message& get() const { return static_cast< const concrete_message&>( *this);}

               };


            } // detail


            namespace transaction
            {
               namespace resource
               {
                  template< typename Wrapper>
                  struct basic_request : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        //base_type::marshal( archive);
                        archive & this->get().execution;
                        archive & this->get().trid.xid;
                        archive & this->get().resource;
                        archive & this->get().flags;
                     })
                  };
                  template< typename Wrapper>
                  struct basic_reply : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        //base_type::marshal( archive);
                        archive & this->get().execution;
                        archive & this->get().trid.xid;
                        archive & this->get().resource;
                        archive & this->get().state;
                     })
                  };




                  namespace send
                  {
                     template< typename Message>
                     using basic_request = resource::basic_request< detail::external_send_wrapper< Message>>;

                     template< typename Message>
                     using basic_reply = resource::basic_reply< detail::external_send_wrapper< Message>>;

                     namespace prepare
                     {
                        using Request = basic_request< common::message::transaction::resource::prepare::Request>;
                        using Reply = basic_reply< common::message::transaction::resource::prepare::Reply>;
                     } // prepare

                     namespace commit
                     {
                        using Request = basic_request< common::message::transaction::resource::commit::Request>;
                        using Reply = basic_reply< common::message::transaction::resource::commit::Reply>;
                     } // commit

                     namespace rollback
                     {
                        using Request = basic_request< common::message::transaction::resource::rollback::Request>;
                        using Reply = basic_reply< common::message::transaction::resource::rollback::Reply>;
                     } // rollback

                  } // send

                  namespace receive
                  {
                     template< typename Message>
                     using basic_request = resource::basic_request< detail::external_receive_wrapper< Message>>;

                     template< typename Message>
                     using basic_reply = resource::basic_reply< detail::external_receive_wrapper< Message>>;

                     namespace prepare
                     {
                        using Request = basic_request< common::message::transaction::resource::prepare::Request>;
                        using Reply = basic_reply< common::message::transaction::resource::prepare::Reply>;

                     } // prepare

                     namespace commit
                     {
                        using Request = basic_request< common::message::transaction::resource::commit::Request>;
                        using Reply = basic_reply< common::message::transaction::resource::commit::Reply>;

                     } // commit

                     namespace rollback
                     {
                        using Request = basic_request< common::message::transaction::resource::rollback::Request>;
                        using Reply = basic_reply< common::message::transaction::resource::rollback::Reply>;

                     } // rollback
                  } // receive

               } // resource
            } // transaction

            namespace service
            {
               namespace call
               {
                  template< typename Wrapper>
                  struct basic_request : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().descriptor;
                        archive & this->get().service.name;
                        archive & this->get().service.timeout;
                        archive & this->get().parent;

                        archive & this->get().trid.xid;
                        archive & this->get().flags;

                        archive & this->get().buffer;
                     })
                  };

                  template< typename Wrapper>
                  struct basic_reply : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().descriptor;
                        archive & this->get().error;
                        archive & this->get().code;

                        archive & this->get().transaction.trid.xid;
                        archive & this->get().transaction.state;

                        archive & this->get().buffer;
                     })
                  };

                  namespace send
                  {
                     using Request = basic_request< detail::external_send_wrapper< common::message::service::call::callee::Request>>;
                     using Reply = basic_reply< detail::external_send_wrapper< common::message::service::call::Reply>>;

                  } // send

                  namespace receive
                  {
                     using Request = basic_request< detail::external_receive_wrapper< common::message::service::call::callee::Request>>;
                     using Reply = basic_reply< detail::external_receive_wrapper< common::message::service::call::Reply>>;
                  } // receive

               } // call
            } // service


            namespace domain
            {
               namespace discovery
               {
                  template< typename Wrapper>
                  struct basic_request : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().domain;
                        archive & this->get().services;
                     })
                  };

                  template< typename Wrapper>
                  struct basic_reply : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().domain;
                        archive & this->get().services;
                     })
                  };

                  namespace send
                  {
                     using Request = basic_request< detail::external_send_wrapper< common::message::gateway::domain::discover::internal::Request>>;
                     using Reply = basic_reply< detail::external_send_wrapper< common::message::gateway::domain::discover::internal::Reply>>;

                  } // send

                  namespace receive
                  {
                     using Request = basic_request< detail::external_receive_wrapper< common::message::gateway::domain::discover::internal::Request>>;
                     using Reply = basic_reply< detail::external_receive_wrapper< common::message::gateway::domain::discover::internal::Reply>>;
                  } // receive

               } // discovery

            } // domain

            //!
            //! Helpers to get the right interdomain wrapper before sending, hence correct marshaling to the other domain.
            //!
            namespace send
            {
               inline transaction::resource::send::prepare::Request wrap( common::message::transaction::resource::prepare::Request& message)
               { return { message};}

               inline transaction::resource::send::prepare::Reply wrap( common::message::transaction::resource::prepare::Reply& message)
               { return { message};}


               inline transaction::resource::send::commit::Request wrap( common::message::transaction::resource::commit::Request& message)
               { return { message};}

               inline transaction::resource::send::commit::Reply wrap( common::message::transaction::resource::commit::Reply& message)
               { return { message};}


               inline transaction::resource::send::rollback::Request wrap( common::message::transaction::resource::rollback::Request& message)
               { return { message};}

               inline transaction::resource::send::rollback::Reply wrap( common::message::transaction::resource::rollback::Reply& message)
               { return { message};}

               inline service::call::send::Request wrap( common::message::service::call::callee::Request& message)
               { return { message};}

               inline service::call::send::Reply wrap( common::message::service::call::Reply& message)
               { return { message};}


               inline domain::discovery::send::Request wrap( common::message::gateway::domain::discover::internal::Request& message)
               { return { message};}

               inline domain::discovery::send::Reply wrap( common::message::gateway::domain::discover::internal::Reply& message)
               { return { message};}

            } // send
         } // interdomain
      } // message
   } // gateway

   namespace common
   {
      namespace message
      {
         namespace reverse
         {
            template<>
            struct type_traits< casual::gateway::message::ipc::connect::Request> : detail::type< casual::gateway::message::ipc::connect::Reply> {};


            template<>
            struct type_traits< casual::gateway::message::interdomain::service::call::receive::Request>
            : detail::type< casual::gateway::message::interdomain::service::call::receive::Reply> {};


         } // reverse
      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
