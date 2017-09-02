//!
//! casual 
//!

#include "gateway/message.h"

#include "common/communication/message.h"
#include "common/marshal/network.h"
#include "common/marshal/complete.h"

#include "common/arguments.h"

#include <fstream>

namespace casual
{
   namespace gateway
   {
      namespace protocol
      {
         namespace local
         {
            namespace
            {

               void write( const common::communication::message::Complete& complete, std::ostream& out)
               {
                  auto header = complete.header();
                  out.write( reinterpret_cast< const char*>( &header), common::communication::message::complete::network::header::size());

                  out.write( complete.payload.data(), complete.payload.size());
               }

               template< typename M>
               void generate( M&& message, const std::string& filename)
               {
                  std::ofstream file{ filename + '.' + std::to_string( common::cast::underlying( common::message::type( message))) + ".bin", std::ios::binary | std::ios::trunc};

                  write( common::marshal::complete( message, common::marshal::binary::network::create::Output{}), file);
               }


               template< typename M>
               void set_general( M& message)
               {
                  message.correlation = common::Uuid{ "5b6c1bf6f24b480dbdbcdef54c3a0857"};
                  message.execution = common::Uuid{ "7073cbf414444a4187b30086f143fc60"};
               }

               common::transaction::ID trid()
               {
                  return {
                     common::Uuid{ "5b6c1bf6f24b480dbdbcdef54c3a0851"},
                     common::Uuid{ "5b6c1bf6f24b480dbdbcdef54c3a0852"},
                     common::process::Handle{ common::platform::process::id{ 42}, common::platform::ipc::id{ 42}}
                  };
               }


               void generate( const std::string& basename)
               {
                  using version_type = common::message::gateway::domain::protocol::Version;
                  {
                     common::message::gateway::domain::connect::Request message;
                     set_general( message);

                     message.domain.id = common::Uuid{ "315dacc6182e4c12bf9877efa924cb86"};
                     message.domain.name = "domain A";
                     message.versions = { version_type::version_1};

                     generate( message, basename + "message.gateway.domain.connect.Request");
                  }

                  {
                     common::message::gateway::domain::connect::Reply message;
                     set_general( message);

                     message.domain.id = common::Uuid{ "315dacc6182e4c12bf9877efa924cb86"};
                     message.domain.name = "domain A";
                     message.version = version_type::version_1;

                     generate( message, basename + "message.gateway.domain.connect.Reply");
                  }

                  {
                     common::message::gateway::domain::discover::Request message;
                     set_general( message);

                     message.domain.id = common::Uuid{ "315dacc6182e4c12bf9877efa924cb86"};
                     message.domain.name = "domain A";

                     message.services = { "service1", "service2", "service3"};
                     message.queues = { "queue1", "queue2", "queue3"};

                     generate( message, basename + "message.gateway.domain.discovery.Request");
                  }

                  {
                     common::message::gateway::domain::discover::Reply message;
                     set_general( message);

                     message.domain.id = common::Uuid{ "e2f6b7c37f734a0982a0ab1581b21fa5"};
                     message.domain.name = "domain B";

                     message.services = {
                           { []( auto& s){
                              s.name = "service1";
                              s.category = "example";
                              s.transaction = common::service::transaction::Type::join;
                              s.timeout = std::chrono::seconds{ 90};
                           }}
                     };
                     message.queues = {
                           { []( auto& q){
                              q.name = "queue1";
                              q.retries = 10;
                           }}
                     };

                     generate( message, basename + "message.gateway.domain.discovery.Reply");
                  }

                  {
                     common::message::service::call::callee::Request message;
                     set_general( message);

                     message.service.name = "service1";
                     message.service.timeout = std::chrono::seconds{ 42};

                     message.parent = "parent-service";
                     message.trid = trid();

                     message.flags = common::message::service::call::request::Flag::no_reply;
                     message.buffer.type = ".json/";
                     message.buffer.memory = { '{', '}'};


                     generate( message, basename + "message.service.call.Request");
                  }

                  {
                     common::message::service::call::Reply message;
                     set_general( message);

                     message.status = common::code::xatmi::service_fail;
                     message.code = 42;
                     message.transaction.trid = trid();
                     message.transaction.state = common::message::service::Transaction::State::active;

                     message.buffer.type = ".json/";
                     message.buffer.memory = { '{', '}'};

                     generate( message, basename + "message.service.call.Reply");
                  }

                  {
                     common::message::conversation::connect::callee::Request message;
                     set_general( message);

                     message.service.name = "service1";
                     message.service.timeout = std::chrono::seconds{ 42};

                     message.parent = "parent-service";
                     message.trid = trid();

                     message.recording.nodes.emplace_back( common::platform::ipc::id{ 42});
                     message.recording.nodes.emplace_back( common::platform::ipc::id{ 4242});

                     message.flags = common::flag::service::conversation::connect::Flag::send_only;
                     message.buffer.type = ".json/";
                     message.buffer.memory = { '{', '}'};


                     generate( message, basename + "message.conversation.connect.Request");
                  }

                  {
                     common::message::conversation::connect::Reply message;
                     set_general( message);

                     message.route.nodes.emplace_back( common::platform::ipc::id{ 4242});
                     message.route.nodes.emplace_back( common::platform::ipc::id{ 42});

                     message.recording.nodes.emplace_back( common::platform::ipc::id{ 42});
                     message.recording.nodes.emplace_back( common::platform::ipc::id{ 4242});

                     


                     generate( message, basename + "message.conversation.connect.Reply");
                  }

                  {
                     common::message::conversation::callee::Send message;
                     set_general( message);

                     message.route.nodes.emplace_back( common::platform::ipc::id{ 42});
                     message.route.nodes.emplace_back( common::platform::ipc::id{ 4242});


                     message.events = common::flag::service::conversation::Event::send_only;
                     message.status = common::code::xatmi::ok;
                     message.flags = common::flag::service::conversation::send::Flag::no_time;

                     message.buffer.type = ".json/";
                     message.buffer.memory = { '{', '}'};

                     message.status = common::code::xatmi::ok;


                     generate( message, basename + "message.conversation.Send");
                  }

                  {
                     common::message::conversation::Disconnect message;
                     set_general( message);

                     message.route.nodes.emplace_back( common::platform::ipc::id{ 42});
                     message.route.nodes.emplace_back( common::platform::ipc::id{ 4242});

                     message.events = common::flag::service::conversation::Event::send_only;

                     generate( message, basename + "message.conversation.Disconnect");
                  }

                  {
                     common::message::queue::enqueue::Request message;
                     set_general( message);

                     message.name = "queueA";
                     message.trid = trid();

                     message.message.id = common::uuid::make();
                     message.message.properties = { "property 1", "property 2"};
                     message.message.reply = "queueB";
                     message.message.available = common::platform::time::clock::type::now();
                     message.message.type = ".json/";
                     message.message.payload = { '{', '}'};


                     generate( message, basename + "message.queue.enqueue.Request");
                  }

                  {
                     common::message::queue::enqueue::Reply message;
                     set_general( message);

                     message.id = common::Uuid{ "315dacc6182e4c12bf9877efa924cb87"};

                     generate( message, basename + "message.queue.enqueue.Reply");
                  }

                  {
                     common::message::queue::dequeue::Request message;
                     set_general( message);

                     message.name = "queueA";
                     message.trid = trid();

                     message.selector.properties = { "property 1", "property 2"};
                     message.selector.id = common::Uuid{ "315dacc6182e4c12bf9877efa924cb87"};

                     message.block = false;
                     generate( message, basename + "message.queue.dequeue.Request");
                  }

                  {
                     common::message::queue::dequeue::Reply reply;
                     set_general( reply);
                     
                     reply.message.resize( 1);
                     auto& message = reply.message.back();


                     message.id = common::uuid::make();
                     message.properties = { "property 1", "property 2"};
                     message.reply = "queueB";
                     message.available = common::platform::time::clock::type::now();
                     message.type = ".json/";
                     message.payload = { '{', '}'};
                     message.redelivered = 1;
                     message.timestamp = common::platform::time::clock::type::now();

                     generate( reply, basename + "message.queue.dequeue.Reply");
                  }

                  {
                     auto transaction_request = []( auto& message){
                        set_general( message);
                        message.trid = trid();
                        message.resource = 42;
                        message.flags = common::flag::xa::Flag::no_flags;
                     };

                     auto transaction_reply = []( auto& message){
                        set_general( message);
                        message.trid = trid();
                        message.resource = 42;
                        message.state = common::code::xa::ok;
                     };

                     {
                        common::message::transaction::resource::prepare::Request message;
                        transaction_request( message);
                        generate( message, basename + "message.transaction.resource.prepare.Request");
                     }

                     {
                        common::message::transaction::resource::prepare::Reply message;
                        transaction_reply( message);
                        generate( message, basename + "message.transaction.resource.prepare.Reply");
                     }

                     {
                        common::message::transaction::resource::commit::Request message;
                        transaction_request( message);
                        generate( message, basename + "message.transaction.resource.commit.Request");
                     }

                     {
                        common::message::transaction::resource::commit::Reply message;
                        transaction_reply( message);
                        generate( message, basename + "message.transaction.resource.commit.Reply");
                     }

                     {
                        common::message::transaction::resource::rollback::Request message;
                        transaction_request( message);
                        generate( message, basename + "message.transaction.resource.rollback.Request");
                     }

                     {
                        common::message::transaction::resource::rollback::Reply message;
                        transaction_reply( message);
                        generate( message, basename + "message.transaction.resource.rollback.Reply");
                     }
                  }
               }

               int main(int argc, char **argv)
               {
                  try
                  {
                     std::string basename;

                     {
                        common::Arguments arguments{
                           {
                              common::argument::directive( { "-b", "--base"}, "base path for the generated blobs", basename)
                           }
                        };

                        arguments.parse( argc, argv);
                     }

                     if( common::range::back( basename) != '/')
                        basename.push_back( '/');

                     generate( basename);
                     return 0;
                  }
                  catch( ...)
                  {
                     return common::exception::handle( std::cerr);
                  }
               }
            } // <unnamed>
         } // local

      } // protocol

   } // gateway
} // casual

int main(int argc, char **argv)
{
   return casual::gateway::protocol::local::main( argc, argv);
}

