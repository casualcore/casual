//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/documentation/protocol/example.h"
#include "gateway/message.h"


#include "common/communication/tcp/message.h"
#include "common/serialize/native/network.h"
#include "common/serialize/native/complete.h"
#include "common/serialize/create.h"

#include "common/argument.h"
#include "common/string.h"
#include "common/exception/guard.h"

#include <fstream>

namespace casual
{
   using namespace common;

   namespace gateway::documentation::protocol
   {
      namespace local
      {
         namespace
         {
            template< typename M, typename E>
            auto file( M&& message, std::filesystem::path base, E&& extension)
            {
               auto information = common::string::compose( 
                  '.', gateway::message::domain::protocol::Version::version_1,
                  '.', cast::underlying( common::message::type( message)),
                  '.', extension);

               return std::ofstream{ base += information, std::ios::binary | std::ios::trunc};
            }

            using complete_type = communication::tcp::message::Complete;

            namespace generator
            {
               auto binary = []( auto&& message, const std::filesystem::path& base)
               {
                  auto file = local::file( message, base, "bin");

                  auto complete = serialize::native::complete< complete_type>( message);

                  auto header = complete.header();
                  file.write( reinterpret_cast< const char*>( &header), communication::tcp::message::header::size);
                  file.write( complete.payload.data(), complete.payload.size());
               };

               //! Local 'wrapper' writer to enable _network normalizing_, so we only serialize the
               //! properties that are transported over the wire.
               struct Writer : serialize::Writer
               {
                  using is_network_normalizing = void; // this is what trigger the 'specialization'
                  Writer( serialize::Writer writer) : serialize::Writer{ std::move( writer)} {}

                  //! we need to implement operator << so this type is returned, hence, we keep the
                  //! is_network_normalizing throughout the serialization.
                  template< typename V>
                  Writer& operator << ( V&& value)
                  {
                     serialize::value::write( *this, std::forward< V>( value), nullptr);
                     return *this;
                  }
               };

               auto descriptive = []( auto format)
               {
                  return [format]( auto&& message, const std::filesystem::path& base)
                  {
                     generator::Writer archive{ serialize::create::writer::from( format)};
                     archive << CASUAL_NAMED_VALUE( message);
                     auto file = local::file( message, base, format);
                     archive.consume( file);
                  };
               };
            } // generator


            template< typename G>
            void generate( G&& generator, const std::filesystem::path& basename)
            {
               generator( example::message< gateway::message::domain::connect::Request>(), basename / "message.gateway.domain.connect.Request");
               generator( example::message< gateway::message::domain::connect::Reply>(), basename / "message.gateway.domain.connect.Reply");
               generator( example::message< gateway::message::domain::discovery::Request>(), basename / "message.gateway.domain.discovery.Request");
               generator( example::message< gateway::message::domain::discovery::Reply>(), basename / "message.gateway.domain.discovery.Reply");
               generator( example::message< common::message::service::call::callee::Request>(), basename / "message.service.call.Request");
               generator( example::message< common::message::service::call::Reply>(), basename / "message.service.call.Reply");
               generator( example::message< common::message::conversation::connect::callee::Request>(), basename / "message.conversation.connect.Request");
               generator( example::message< common::message::conversation::connect::Reply>(), basename / "message.conversation.connect.Reply");
               generator( example::message< common::message::conversation::callee::Send>(), basename / "message.conversation.Send");
               generator( example::message< common::message::conversation::Disconnect>(), basename / "message.conversation.Disconnect");
               generator( example::message< queue::ipc::message::group::enqueue::Request>(), basename / "message.queue.enqueue.Request");
               generator( example::message< queue::ipc::message::group::enqueue::Reply>(), basename / "message.queue.enqueue.Reply");
               generator( example::message< queue::ipc::message::group::dequeue::Request>(), basename / "message.queue.dequeue.Request");
               generator( example::message< queue::ipc::message::group::dequeue::Reply>(), basename / "message.queue.dequeue.Reply");
               generator( example::message< common::message::transaction::resource::prepare::Request>(), basename / "message.transaction.resource.prepare.Request");
               generator( example::message< common::message::transaction::resource::prepare::Reply>(), basename / "message.transaction.resource.prepare.Reply");
               generator( example::message< common::message::transaction::resource::commit::Request>(), basename / "message.transaction.resource.commit.Request");
               generator( example::message< common::message::transaction::resource::commit::Reply>(), basename / "message.transaction.resource.commit.Reply");
               generator( example::message< common::message::transaction::resource::rollback::Request>(), basename / "message.transaction.resource.rollback.Request");
               generator( example::message< common::message::transaction::resource::rollback::Reply>(), basename / "message.transaction.resource.rollback.Reply");
            }

            void main(int argc, char **argv)
            {
               std::filesystem::path basename;
               std::string format;

               {
                  using namespace casual::common::argument;
                  Parse{ R"(binary dump examples for interdomain protocol

generated files will have the format:

bin-dump:    [<base-path>/]<message-name>.<protocol-version>.<message-type-id>.bin
descriptive: [<base-path>/]<message-name>.<protocol-version>.<message-type-id>.<format>

)",
                     Option( std::tie( basename), { "-b", "--base"}, "base path for the generated files"),
                     Option( std::tie( format), common::serialize::create::writer::complete::format(), { "--format"}, "format for optional descriptive generated representation")
                  }( argc, argv);
               }

               // generate the binary blobs   
               generate( generator::binary, basename);

               if( ! format.empty())
                  generate( generator::descriptive( std::move( format)), basename);

            }
         } // <unnamed>
      } // local
   } // gateway::documentation::protocol


} // casual

int main(int argc, char **argv)
{
   return casual::common::exception::main::cli::guard( [&]()
   {
      casual::gateway::documentation::protocol::local::main( argc, argv);
   });
}

