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
#include "common/message/queue.h"
#include "common/message/conversation.h"
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
            common::domain::Identity domain;
            std::vector< std::string> address;

            CASUAL_CONST_CORRECT_MARSHAL({
               common::message::basic_message< type>::marshal( archive);
               archive & process;
               archive & domain;
               archive & address;
            })

            friend std::ostream& operator << ( std::ostream& out, const basic_connect& value)
            {
               return out << "{ process: " << value.process
                     << ", address: " << common::range::make( value.address)
                     << ", domain: " << value.domain
                     << '}';
            }
         };

         namespace outbound
         {
            namespace configuration
            {

               struct Request : common::message::server::basic_id< common::message::Type::gateway_outbound_configuration_request>
               {

                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };


               using base_reply = common::message::server::basic_id< common::message::Type::gateway_outbound_configuration_reply>;
               struct Reply : base_reply
               {
                  std::vector< std::string> services;
                  std::vector< std::string> queues;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_reply::marshal( archive);
                     archive & services;
                     archive & queues;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };

            } // configuration

            struct Connect : basic_connect< common::message::Type::gateway_outbound_connect>
            {
            };

         } // outbound

         namespace inbound
         {
            using base_connect =  basic_connect< common::message::Type::gateway_inbound_connect>;
            struct Connect : base_connect
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
            namespace connect
            {
               struct Limit
               {
                  std::size_t size = 0;
                  std::size_t messages = 0;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     archive & size;
                     archive & messages;
                  )

               };
            } // connect
            struct Connect : common::message::basic_message< common::message::Type::gateway_manager_tcp_connect>
            {
               common::platform::tcp::descriptor::type descriptor;
               connect::Limit limit;

               friend std::ostream& operator << ( std::ostream& out, const Connect& value);

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & descriptor;
                  archive & limit;
               })
            };

         } // tcp


         namespace worker
         {

            struct Connect : common::message::basic_message< common::message::Type::gateway_worker_connect>
            {
               common::platform::binary::type information;

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
               { return common::message::Type::interdomain_transaction_resource_prepare_request;}

               constexpr common::message::Type message_type( common::message::transaction::resource::prepare::Reply&&)
               { return common::message::Type::interdomain_transaction_resource_prepare_reply;}

               constexpr common::message::Type message_type( common::message::transaction::resource::commit::Request&&)
               { return common::message::Type::interdomain_transaction_resource_commit_request;}

               constexpr common::message::Type message_type( common::message::transaction::resource::commit::Reply&&)
               { return common::message::Type::interdomain_transaction_resource_commit_reply;}

               constexpr common::message::Type message_type( common::message::transaction::resource::rollback::Request&&)
               { return common::message::Type::interdomain_transaction_resource_rollback_request;}

               constexpr common::message::Type message_type( common::message::transaction::resource::rollback::Reply&&)
               { return common::message::Type::interdomain_transaction_resource_rollback_reply;}


               constexpr common::message::Type message_type( common::message::service::call::callee::Request&&)
               { return common::message::Type::interdomain_service_call;}

               constexpr common::message::Type message_type( common::message::service::call::Reply&&)
               { return common::message::Type::interdomain_service_reply;}


               constexpr common::message::Type message_type( common::message::conversation::connect::callee::Request&&)
               { return common::message::Type::interdomain_conversation_connect_request;}

               constexpr common::message::Type message_type( common::message::conversation::connect::Reply&&)
               { return common::message::Type::interdomain_conversation_connect_reply;}

               constexpr common::message::Type message_type( common::message::conversation::callee::Send&&)
               { return common::message::Type::interdomain_conversation_send;}

               constexpr common::message::Type message_type( common::message::conversation::Disconnect&&)
               { return common::message::Type::interdomain_conversation_disconnect;}


               constexpr common::message::Type message_type( common::message::gateway::domain::discover::Request&&)
               { return common::message::Type::interdomain_domain_discover_request;}

               constexpr common::message::Type message_type( common::message::gateway::domain::discover::Reply&&)
               { return common::message::Type::interdomain_domain_discover_reply;}


               constexpr common::message::Type message_type( common::message::queue::enqueue::Request&&)
               { return common::message::Type::interdomain_queue_enqueue_request;}

               constexpr common::message::Type message_type( common::message::queue::enqueue::Reply&&)
               { return common::message::Type::interdomain_queue_enqueue_reply;}

               constexpr common::message::Type message_type( common::message::queue::dequeue::Request&&)
               { return common::message::Type::interdomain_queue_dequeue_request;}

               constexpr common::message::Type message_type( common::message::queue::dequeue::Reply&&)
               { return common::message::Type::interdomain_queue_dequeue_reply;}


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
                        archive & this->get().status;
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

               namespace conversation
               {
                  namespace connect
                  {
                     template< typename Wrapper>
                     struct basic_request : call::basic_request< Wrapper>
                     {
                        using base_request = call::basic_request< Wrapper>;
                        using base_request::base_request;

                        CASUAL_CONST_CORRECT_MARSHAL({
                           base_request::marshal( archive);
                           archive & this->get().recording;
                        })
                     };

                     template< typename Wrapper>
                     struct basic_reply : Wrapper
                     {
                        using Wrapper::Wrapper;

                        CASUAL_CONST_CORRECT_MARSHAL({
                           archive & this->get().execution;
                           archive & this->get().route;
                           archive & this->get().recording;
                           archive & this->get().status;
                        })
                     };

                     namespace send
                     {
                        using Request = basic_request< detail::external_send_wrapper< common::message::conversation::connect::callee::Request>>;
                        using Reply = basic_reply< detail::external_send_wrapper< common::message::conversation::connect::Reply>>;
                     } // send

                     namespace receive
                     {
                        using Request = basic_request< detail::external_receive_wrapper< common::message::conversation::connect::callee::Request>>;
                        using Reply = basic_reply< detail::external_receive_wrapper< common::message::conversation::connect::Reply>>;
                     } // receive

                  } // connect

                  namespace send
                  {
                     template< typename Wrapper>
                     struct basic_request : call::basic_request< Wrapper>
                     {
                        using base_request = call::basic_request< Wrapper>;
                        using base_request::base_request;

                        CASUAL_CONST_CORRECT_MARSHAL({
                           base_request::marshal( archive);
                           archive & this->get().recording;
                        })
                     };

                  } // send

               } // conversation
            } // service

            namespace queue
            {
               namespace enqueue
               {
                  template< typename Wrapper>
                  struct basic_request : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().name;
                        archive & this->get().trid.xid;
                        archive & this->get().message;
                     })
                  };

                  template< typename Wrapper>
                  struct basic_reply : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().id;
                     })
                  };

                  namespace send
                  {
                     using Request = basic_request< detail::external_send_wrapper< common::message::queue::enqueue::Request>>;
                     using Reply = basic_reply< detail::external_send_wrapper< common::message::queue::enqueue::Reply>>;

                  } // send

                  namespace receive
                  {
                     using Request = basic_request< detail::external_receive_wrapper< common::message::queue::enqueue::Request>>;
                     using Reply = basic_reply< detail::external_receive_wrapper< common::message::queue::enqueue::Reply>>;
                  } // receive


               } // enqueue

               namespace dequeue
               {

                  template< typename Wrapper>
                  struct basic_request : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().name;
                        archive & this->get().trid.xid;
                        archive & this->get().selector;
                        archive & this->get().block;
                     })
                  };

                  template< typename Wrapper>
                  struct basic_reply : Wrapper
                  {
                     using Wrapper::Wrapper;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        archive & this->get().execution;
                        archive & this->get().message;
                     })
                  };


                  namespace send
                  {
                     using Request = basic_request< detail::external_send_wrapper< common::message::queue::dequeue::Request>>;
                     using Reply = basic_reply< detail::external_send_wrapper< common::message::queue::dequeue::Reply>>;

                  } // send

                  namespace receive
                  {
                     using Request = basic_request< detail::external_receive_wrapper< common::message::queue::dequeue::Request>>;
                     using Reply = basic_reply< detail::external_receive_wrapper< common::message::queue::dequeue::Reply>>;
                  } // receive

               } // dequeue

            } // queue


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
                        archive & this->get().queues;
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
                        archive & this->get().queues;
                     })
                  };

                  namespace send
                  {
                     using Request = basic_request< detail::external_send_wrapper< common::message::gateway::domain::discover::Request>>;
                     using Reply = basic_reply< detail::external_send_wrapper< common::message::gateway::domain::discover::Reply>>;

                  } // send

                  namespace receive
                  {
                     using Request = basic_request< detail::external_receive_wrapper< common::message::gateway::domain::discover::Request>>;
                     using Reply = basic_reply< detail::external_receive_wrapper< common::message::gateway::domain::discover::Reply>>;
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


               inline service::conversation::connect::send::Request wrap( common::message::conversation::connect::callee::Request& message)
               { return { message};}

               inline service::conversation::connect::send::Reply wrap( common::message::conversation::connect::Reply& message)
               { return { message};}


               inline domain::discovery::send::Request wrap( common::message::gateway::domain::discover::Request& message)
               { return { message};}

               inline domain::discovery::send::Reply wrap( common::message::gateway::domain::discover::Reply& message)
               { return { message};}

               inline queue::dequeue::send::Request wrap( common::message::queue::dequeue::Request& message)
               { return { message};}

               inline queue::dequeue::send::Reply wrap( common::message::queue::dequeue::Reply& message)
               { return { message};}

               inline queue::enqueue::send::Request wrap( common::message::queue::enqueue::Request& message)
               { return { message};}

               inline queue::enqueue::send::Reply wrap( common::message::queue::enqueue::Reply& message)
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


            template<>
            struct type_traits< casual::gateway::message::outbound::configuration::Request> : detail::type< casual::gateway::message::outbound::configuration::Reply> {};


         } // reverse
      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
