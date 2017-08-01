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
                  std::ofstream file{ filename, std::ios::binary | std::ios::trunc};

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
                     common::process::Handle{ 42, common::communication::ipc::Handle{ 42}}
                  };
               }


               void generate( const std::string& basename)
               {
                  {
                     common::message::gateway::domain::discover::Request message;
                     set_general( message);

                     message.domain.id = common::Uuid{ "315dacc6182e4c12bf9877efa924cb86"};
                     message.domain.name = "domain A";

                     message.services = { "service1", "service2", "service3"};
                     message.queues = { "queue1", "queue2", "queue3"};

                     generate( message, basename + "message.interdomain.domain.discovery.receive.Request.bin");
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

                     generate( message, basename + "message.interdomain.domain.discovery.receive.Reply.bin");
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


                     generate( message, basename + "message.interdomain.service.call.receive.Request.bin");
                  }

                  {
                     common::message::service::call::Reply message;
                     set_general( message);

                     message.status = common::error::code::xatmi::service_fail;
                     message.code = 42;
                     message.transaction.trid = trid();
                     message.transaction.state = common::message::service::Transaction::State::active;

                     message.buffer.type = ".json/";
                     message.buffer.memory = { '{', '}'};

                     generate( message, basename + "message.interdomain.service.call.receive.Reply.bin");
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
                        message.state = common::error::code::xa::ok;
                     };

                     {
                        common::message::transaction::resource::prepare::Request message;
                        transaction_request( message);
                        generate( message, basename + "message.interdomain.transaction.resource.receive.prepare.Request.bin");
                     }

                     {
                        common::message::transaction::resource::prepare::Reply message;
                        transaction_reply( message);
                        generate( message, basename + "message.interdomain.transaction.resource.receive.prepare.Reply.bin");
                     }

                     {
                        common::message::transaction::resource::commit::Request message;
                        transaction_request( message);
                        generate( message, basename + "message.interdomain.transaction.resource.receive.commit.Request.bin");
                     }

                     {
                        common::message::transaction::resource::commit::Reply message;
                        transaction_reply( message);
                        generate( message, basename + "message.interdomain.transaction.resource.receive.commit.Reply.bin");
                     }

                     {
                        common::message::transaction::resource::rollback::Request message;
                        transaction_request( message);
                        generate( message, basename + "message.interdomain.transaction.resource.receive.rollback.Request.bin");
                     }

                     {
                        common::message::transaction::resource::rollback::Reply message;
                        transaction_reply( message);
                        generate( message, basename + "message.interdomain.transaction.resource.receive.rollback.Reply.bin");
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

