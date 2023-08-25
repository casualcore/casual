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
            template< typename M>
            auto file( M&& message, std::filesystem::path base, std::string_view extension)
            {
               if( ! std::filesystem::exists( base))
                  std::filesystem::create_directories( base);

               std::filesystem::path information = common::string::compose(
                  common::message::type( message), 
                  '.', message::protocol::version< M>(),
                  '.', cast::underlying( common::message::type( message)),
                  '.', extension);
               
               return std::ofstream{ base / information, std::ios::binary | std::ios::trunc};
            }

            using complete_type = communication::tcp::message::Complete;

            namespace generator
            {
               auto binary = []( auto&& message, const std::filesystem::path& base)
               {
                  auto file = local::file( message, base, "bin");

                  auto complete = serialize::native::complete< complete_type>( message);

                  file.write( complete.header_data(), communication::tcp::message::header::size);
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
               generator( example::message< gateway::message::domain::connect::Request>(), basename);
               generator( example::message< gateway::message::domain::connect::Reply>(), basename);
               generator( example::message< casual::gateway::message::domain::disconnect::Request>(), basename);
               generator( example::message< casual::gateway::message::domain::disconnect::Reply>(), basename);
               generator( example::message< gateway::message::domain::disconnect::Request>(), basename);
               generator( example::message< gateway::message::domain::disconnect::Reply>(), basename);
               generator( example::message< casual::domain::message::discovery::Request>(), basename);
               generator( example::message< casual::domain::message::discovery::Reply>(), basename);
               generator( example::message< casual::domain::message::discovery::topology::implicit::Update>(), basename);
               generator( example::message< common::message::service::call::callee::Request>(), basename);
               generator( example::message< common::message::service::call::Reply>(), basename);
               generator( example::message< common::message::conversation::connect::callee::Request>(), basename);
               generator( example::message< common::message::conversation::connect::Reply>(), basename);
               generator( example::message< common::message::conversation::callee::Send>(), basename);
               generator( example::message< common::message::conversation::Disconnect>(), basename);
               generator( example::message< queue::ipc::message::group::enqueue::Request>(), basename);
               generator( example::message< queue::ipc::message::group::enqueue::Reply>(), basename);
               generator( example::message< queue::ipc::message::group::dequeue::Request>(), basename);
               generator( example::message< queue::ipc::message::group::dequeue::Reply>(), basename);
               generator( example::message< common::message::transaction::resource::prepare::Request>(), basename);
               generator( example::message< common::message::transaction::resource::prepare::Reply>(), basename);
               generator( example::message< common::message::transaction::resource::commit::Request>(), basename);
               generator( example::message< common::message::transaction::resource::commit::Reply>(), basename);
               generator( example::message< common::message::transaction::resource::rollback::Request>(), basename);
               generator( example::message< common::message::transaction::resource::rollback::Reply>(), basename);
            }

            void main(int argc, char **argv)
            {
               std::filesystem::path basename;
               std::string format;

               {
                  auto complete = []( auto values, bool help)
                  {
                     return std::vector< std::string>{ "yaml", "json", "xml", "ini"};
                  };

                  using namespace casual::common::argument;
                  Parse{ R"(binary dump examples for interdomain protocol

generated files will have the format:

bin-dump:    [<base-path>/]<message-name>.<protocol-version>.<message-type-id>.bin
descriptive: [<base-path>/]<message-name>.<protocol-version>.<message-type-id>.<format>

)",
                     Option( std::tie( basename), { "-b", "--base"}, "base path for the generated files"),
                     Option( std::tie( format), complete, { "--format"}, "format for optional descriptive generated representation")
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

