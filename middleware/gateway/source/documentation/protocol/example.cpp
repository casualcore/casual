//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/documentation/protocol/example.h"

namespace casual
{
   namespace gateway::documentation::protocol::example
   {
      using namespace common;
      using namespace std::literals;
      namespace detail
      {
         namespace local
         {
            namespace
            {
               namespace indirection
               {
                  template< typename M>
                  auto set_general( M& message, common::traits::priority::tag< 0>)
                  {
                     message.correlation = strong::correlation::id{ 0x5b6c1bf6f24b480dbdbcdef54c3a0857_uuid};
                     message.execution = strong::execution::id{ 0x7073cbf414444a4187b30086f143fc60_uuid};
                  }

                  template< typename M>
                  auto set_general( M& message, common::traits::priority::tag< 1>) -> decltype( message.process, void())
                  {
                     set_general( message, common::traits::priority::tag< 0>{});
                     message.process.pid = common::strong::process::id{ 42};
                     message.process.ipc = common::strong::ipc::id{ 0xcdab6b6fb4744396adc60c29c68c6c1a_uuid};
                  }
               } // indirection

               template< typename M>
               auto set_general( M& message)
               {
                  indirection::set_general( message, common::traits::priority::tag< 1>{});
               }

               common::transaction::ID trid()
               {  
                  return {
                     0x5b6c1bf6f24b480dbdbcdef54c3a0851_uuid,
                     0x5b6c1bf6f24b480dbdbcdef54c3a0852_uuid,
                     common::process::Handle{ common::strong::process::id{ 42}, common::strong::ipc::id{ 0x57c9dcf039dc490baba9b957a39c87f1_uuid}}
                  };
               }

               namespace binary
               {
                  auto value( platform::size::type size)
                  {
                     constexpr auto min = std::numeric_limits< platform::binary::value::type>::min();
                     constexpr auto max = std::numeric_limits< platform::binary::value::type>::max();

                     platform::binary::type result;
                     result.resize( size);

                     auto current = min;

                     common::algorithm::for_each( result, [&current]( auto& c)
                     {
                        c = current;

                        if( current == max)
                           current = min;
                        else 
                           ++current;
                     });

                     return result;
                  }
               } // binary

               namespace time
               {
                  constexpr auto point()
                  {
                     return platform::time::point::type{ std::chrono::microseconds{ 1559762216552100}};
                  }
               } // time

            } // <unnamed>
         } // local
         void fill( gateway::message::domain::connect::Request& message)
         {
            local::set_general( message);

            message.domain.id = common::strong::domain::id{ 0x315dacc6182e4c12bf9877efa924cb86_uuid};
            message.domain.name = "domain A";
            message.versions = common::range::to_vector( gateway::message::protocol::versions);
         }

         void fill( gateway::message::domain::connect::Reply& message)
         {
            local::set_general( message);

            message.domain.id = common::strong::domain::id{ 0x315dacc6182e4c12bf9877efa924cb86_uuid};
            message.domain.name = "domain A";
            message.version = gateway::message::protocol::Version::v1_0;
         }

         void fill( gateway::message::domain::disconnect::Request& message)
         {
            local::set_general( message);
         }

         void fill( gateway::message::domain::disconnect::Reply& message)
         {
            local::set_general( message);
         }

         void fill( casual::domain::message::discovery::Request& message)
         {
            local::set_general( message);

            message.domain.id = common::strong::domain::id{ 0x315dacc6182e4c12bf9877efa924cb86_uuid};
            message.domain.name = "domain A";

            message.content.services( { "service1", "service2", "service3"});
            message.content.queues( { "queue1", "queue2", "queue3"});
         }

         void fill( casual::domain::message::discovery::Reply& message)
         {
            local::set_general( message);

            message.domain.id = common::strong::domain::id{ 0xe2f6b7c37f734a0982a0ab1581b21fa5_uuid};
            message.domain.name = "domain B";

            message.content.services( {
                  [](){
                     casual::domain::message::discovery::reply::Service service;
                     service.name = "service1";
                     service.category = "example";
                     service.transaction = common::service::transaction::Type::join;
                     service.timeout.duration = std::chrono::seconds{ 90};
                     // TODO: 
                     return service;
                  }()
            });
            message.content.queues( {
                  [](){
                     casual::domain::message::discovery::reply::Queue queue;
                     queue.name = "queue1";
                     queue.retries = 10;
                     return queue;
                  }()
            });
         }

         void fill( common::message::service::call::callee::Request& message)
         {
            local::set_general( message);

            message.service.name = "service1";
            message.service.timeout.duration = std::chrono::seconds{ 42};

            message.parent = "parent-service";
            message.trid = local::trid();

            message.flags = common::message::service::call::request::Flag::no_reply;
            message.buffer.type = ".binary/";
            message.buffer.memory = local::binary::value( 128);
         }

         void fill( common::message::service::call::Reply& message)
         {
            local::set_general( message);

            message.code.result = common::code::xatmi::service_fail;
            message.code.user = 42;
            message.transaction.trid = local::trid();
            message.transaction.state = decltype( message.transaction.state)::active;

            message.buffer.type = ".binary/";
            message.buffer.memory = local::binary::value( 128);
         }

         void fill( common::message::conversation::connect::callee::Request& message)
         {
            local::set_general( message);

            message.service.name = "service1";
            message.service.timeout.duration = std::chrono::seconds{ 42};

            message.parent = "parent-service";
            message.trid = local::trid();

            message.duplex = decltype( message.duplex)::send;
            message.buffer.type = ".binary/";
            message.buffer.memory = local::binary::value( 128);
         }

         void fill( common::message::conversation::connect::Reply& message)
         {
            local::set_general( message);

         }

         void fill( common::message::conversation::callee::Send& message)
         {
            local::set_general( message);

            message.duplex = decltype( message.duplex)::send;

            message.buffer.type = ".binary/";
            message.buffer.memory = local::binary::value( 128);

            message.code.result = common::code::xatmi::ok;
            message.code.user = 42;
         }

         void fill( common::message::conversation::Disconnect& message)
         {
            local::set_general( message);
         }

         void fill( casual::queue::ipc::message::group::enqueue::Request& message)
         {
            local::set_general( message);

            message.name = "queueA";
            message.trid = local::trid();

            message.message.id = 0xe6fd9fcf86ac47f4a5252f597e25fc6a_uuid;
            message.message.properties = "property 1:property 2";
            message.message.reply = "queueB";
            message.message.available = local::time::point();
            message.message.type = ".binary/";
            message.message.payload = local::binary::value( 128);
         }

         void fill( casual::queue::ipc::message::group::enqueue::Reply& message)
         {
            local::set_general( message);

            message.id = 0x315dacc6182e4c12bf9877efa924cb87_uuid;
         }

         void fill( casual::queue::ipc::message::group::dequeue::Request& message)
         {
            local::set_general( message);

            message.name = "queueA";
            message.trid = local::trid();

            message.selector.properties = "property 1:property 2";
            message.selector.id = 0x315dacc6182e4c12bf9877efa924cb87_uuid;

            message.block = false;
         }

         void fill( casual::queue::ipc::message::group::dequeue::Reply& message)
         {
            local::set_general( message);

            message.message = { 
               []()
               {
                  casual::queue::ipc::message::group::dequeue::Message message;
                  message.id =  0x532f8b6c15764dca9fe82a3002de579e_uuid;
                  message.properties = "property 1:property 2";
                  message.reply = "queueB";
                  message.available = local::time::point();
                  message.type = ".json/";
                  message.payload = { '{', '}'};
                  message.redelivered = 1;
                  message.timestamp = local::time::point();
                  return message;
               }()
            };
         }

         namespace local
         {
            namespace
            {
               auto transaction_request = []( auto& message){
                  set_general( message);
                  message.trid = trid();
                  message.resource = common::strong::resource::id{ 42};
                  message.flags = common::flag::xa::Flag::no_flags;
               };

               auto transaction_reply = []( auto& message){
                  set_general( message);
                  message.trid = trid();
                  message.resource = common::strong::resource::id{ 42};
                  message.state = common::code::xa::ok;
               };
            } // <unnamed>
         } // local

         void fill( common::message::transaction::resource::prepare::Request& message) { local::transaction_request( message);}
         void fill( common::message::transaction::resource::prepare::Reply& message) { local::transaction_reply( message);}

         void fill( common::message::transaction::resource::commit::Request& message) { local::transaction_request( message);}
         void fill( common::message::transaction::resource::commit::Reply& message) { local::transaction_reply( message);}

         void fill( common::message::transaction::resource::rollback::Request& message) { local::transaction_request( message);}
         void fill( common::message::transaction::resource::rollback::Reply& message) { local::transaction_reply( message);}

      } // detail

   } // gateway::documentation::protocol::example
} // casual