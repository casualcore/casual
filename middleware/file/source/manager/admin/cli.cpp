//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/manager/admin/cli.h"

#include "file/manager/admin/model.h"
#include "file/manager/admin/services.h"
#include "file/message.h"
#include "file/instance.h"

#include "common/log/line.h"
#include "common/terminal.h"
#include "common/communication/instance.h"
#include "common/communication/stream.h"

#include "casual/cli/state.h"
#include "casual/cli/pipe.h"

#include "casual/overloaded.h"

#include "service/protocol/call.h"

#include <iostream>
#include <ranges>

namespace casual
{
   namespace file::manager::admin::cli
   {
      namespace local
      {
         namespace
         {
            namespace detail
            {
               namespace call
               {
                  using Call = casual::service::protocol::binary::Call;

                  manager::admin::model::State state()
                  {
                     return Call{}( 
                        manager::admin::service::name::state).extract< manager::admin::model::State>();
                  }

                  std::vector< common::transaction::global::ID> recover( 
                     std::vector< common::transaction::global::ID> gtrids,
                     manager::admin::model::recovery::Directive directive)
                  {
                     return Call{}( 
                        manager::admin::service::name::recover,
                        std::move( gtrids),
                        std::move( directive)).extract< std::vector< common::transaction::global::ID>>();
                  }
               } // call

               namespace format
               {
                  auto requests( const manager::admin::model::State& state)
                  {
                     common::terminal::format::print( state.requests, 
                        common::terminal::format::column( "path", []( const auto& r){ return r.path;}, common::terminal::color::yellow),
                        common::terminal::format::column( "pid", []( const auto& r){ return r.pid;}, common::terminal::color::white),
                        common::terminal::format::column( "gtrid", []( const auto& r){ return r.gtrid;}),
                        common::terminal::format::column( "stage", []( const auto& r){ return description( r.stage);}),
                        common::terminal::format::column( "time", []( const auto& r){ return std::chrono::floor< std::chrono::microseconds>( r.time);}, common::terminal::color::blue)
                     );
                  }
               } // format

            } // detail

            namespace list
            {
               namespace reservations
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = detail::call::state();
                        detail::format::requests( state);
                     };

                     return argument::Option{
                        std::move( invoke),
                        {{ "-lr", "--list-reservations"}},
                        "list information of files currently reserved"
                     };
                  }
               } // reservations

            } // list

            namespace assets
            {
               
               namespace recovery
               {
                  namespace detail
                  {
                     auto invoke( const manager::admin::model::recovery::Directive directive)
                     {
                        return [directive]( common::transaction::global::ID gtrid, std::vector< common::transaction::global::ID> gtrids)
                        {
                           gtrids.insert( std::begin( gtrids), std::move( gtrid));
                           const auto recovered = local::detail::call::recover( std::move( gtrids), directive); 

                           for( const auto& gtrid : recovered)
                           {
                              common::log::line( std::cout, gtrid);
                           }
                        };
                     }

                     auto complete()
                     {
                        return []( bool help, auto values) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<gtrid>"};

                           std::vector< std::string> result;

                           std::ranges::transform( 
                              local::detail::call::state().requests,
                              std::back_inserter( result),
                              []( const auto& request) { return common::string::compose( request.gtrid); });

                           return result;
                        };
                     }
                  } // detail

                  namespace commit
                  {
                     auto option()
                     {
                        return argument::Option{
                           detail::invoke( manager::admin::model::recovery::Directive::commit),
                           detail::complete(),
                           {{ "--commit"}},
                           "recover global transactions with commit"
                        };
                     }
                  } // commit

                  namespace rollback
                  {
                     auto option()
                     {
                        return argument::Option{
                           detail::invoke( manager::admin::model::recovery::Directive::rollback),
                           detail::complete(),
                           {{ "--rollback"}},
                           "recover global transactions with rollback"
                        };
                     }
                  } // rollback

                  auto option()
                  {
                     return argument::Option{
                        [](){},
                        {{ "--recover-transactions"}},
                        "recover global transactions with --commit or --rollback sub option"
                     }({
                        commit::option(),
                        rollback::option()
                     }, argument::cardinality::one());
                  }
               } // recovery

            } // assets

            namespace pipe
            {
               namespace stem
               {
                  constexpr std::string_view queue = ".queue";
                  constexpr std::string_view payload = ".payload";
                  
               } // stem

               auto reserve_file( const common::transaction::ID& trid, std::filesystem::path path)
               {
                  file::message::reserve::Request request{ common::process::handle()};
                  request.trid = trid;
                  request.path =  std::move( path);
                  request.wait = true;

                  auto reply = common::communication::ipc::call( file::instance::device(), request);
                  
                  if( reply.code != file::code::ok)
                     common::code::raise::error( reply.code, "failed to reserve file: ", request.path);

                  return reply.path;
               }

               struct message_type
               {
                  common::message::Type type{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( type);
                  )
               };

               
               template< typename M>
               struct basic_file_message
               {
                  common::message::Type type{};
                  M message;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( type);
                     CASUAL_SERIALIZE( message);
                  )  
               };
              


               namespace produce
               {
                  // detail to make it clear that it's local to produce
                  namespace detail
                  {
                     struct State
                     {
                        std::filesystem::path directory;
                        common::transaction::ID current;
                        std::string format = "yaml";

                        CASUAL_LOG_SERIALIZE(
                           CASUAL_SERIALIZE( directory);
                           CASUAL_SERIALIZE( current);
                           CASUAL_SERIALIZE( format);
                        )
                     };

                     template< typename M>
                     auto write_file( const detail::State& state, M message, std::filesystem::path path)
                     {
                        common::Trace trace{ "file::manager::admin::cli::local::pipe::produce::detail::write_file"};
                        common::log::debug( "path: ", path);

                        // create the wrapper message to serialize type info
                        basic_file_message< M> file_message{
                           .type = common::message::type( message),
                           .message = std::move( message)
                        };

                        auto archive = common::serialize::create::writer::from( state.format);
                        archive << file_message;

                        auto filename = pipe::reserve_file( state.current, std::move( path));
                        std::ofstream output{ filename};

                        if( ! output)
                           common::code::raise::error( common::code::casual::invalid_file, "failed to open file for writing: ", filename);

                        archive.consume( output);
                     }


                     auto create_handle( const detail::State& state)
                     {
                        return casual::overloaded{
                           [ &state]( casual::cli::message::queue::Message& message)
                           {
                              auto filename = state.directory / common::string::compose( common::chronology::time_point::clock::now(), stem::queue, '.', state.format);
                              write_file( state, std::move( message), std::move( filename));
                           },
                           [ &state]( casual::cli::message::payload::Message& message)
                           {
                              auto filename = state.directory / common::string::compose( common::chronology::time_point::clock::now(), stem::payload, '.', state.format);
                              write_file( state, std::move( message), std::move( filename));

                           }
                        };
                     }

                     auto prepare_directory( std::filesystem::path directory)
                     {
                        if( std::filesystem::exists( directory))
                        {
                           auto status = std::filesystem::status( directory);

                           if( status.type() != std::filesystem::file_type::directory)
                              common::code::raise::error( common::code::casual::invalid_argument, "path exists and is not a directory: ", directory);

                           if( ( status.permissions() & std::filesystem::perms::owner_write) != std::filesystem::perms::owner_write)
                              common::code::raise::error( common::code::casual::constraint_violation, "no write permission for directory: ", directory);
                        }
                        else
                        {
                           std::filesystem::create_directories( directory);
                        }

                        return directory;                     
                     }

                     constexpr std::string_view description = R"(produces files into the provided directory from the pipe
                     
Messages supported are queue messages and payload messages

Has to be used inside a pipe transaction.)";

                     constexpr std::string_view extended = R"(
 Examples:

    `casual transaction --begin \
      | casual queue --consume my-queue \
      | casual file --produce /path/to/directory --format json \
      | casual transaction --commit`

   `casual transaction --begin \
      | casual queue --consume my-queue \
      | casual call --service my-service \
      | casual file --produce /path/to/directory \
      | casual transaction --commit`
)";

                     namespace format
                     {
                        auto option( std::shared_ptr< detail::State>& shared)
                        {
                           auto invoke = [ shared]( std::string format)
                           {
                              shared->format = std::move( format);
                              return argument::option::invoke::preemptive{};
                           };

                           auto complete = []( bool help, auto values) -> std::vector< std::string>
                           {
                              return { "json", "toml", "yaml", "xml"};
                           };

                           return argument::Option{
                              std::move( invoke),
                              std::move( complete),
                              {{ "--format"}},
                              R"(specifies the format of the files - yaml is default)"
                           };

                        }
                     } // format

                  } // detail
           

                  auto option()
                  {
                     auto shared = std::make_shared< detail::State>();

                     auto invoke = [ shared]( std::filesystem::path directory)
                     {
                        common::Trace trace{ "file::manager::admin::cli::local::pipe::produce::invoke"};
                        common::log::debug( "directory: ", directory);

                        // validate or create directory
                        shared->directory = detail::prepare_directory( std::move( directory));

                        // will send Done downstream in dtor
                        casual::cli::pipe::done::Scope done;

                        auto handler = casual::cli::message::dispatch::create( 
                           casual::cli::pipe::forward::handle::defaults(),
                           std::ref( done),
                           // handle current transaction, sets it our current, and forward the message downstream
                           casual::cli::pipe::transaction::handle::current( shared->current),
                           // we will enqueue services replies and queue messages
                           casual::cli::pipe::handle::payloads( detail::create_handle( *shared)));

                        // start the pump
                        common::communication::stream::inbound::Device in{ std::cin};
                        common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);

                        // state.done dtor will send Done downstream
                     };


                     return argument::Option{
                        std::move( invoke),
                        {{ "--produce"}},
                        { detail::description, detail::extended}
                     }( { detail::format::option( shared)}, argument::cardinality::zero_one());

                  }
                  
               } // produce

 
               namespace consume
               {
                  namespace detail
                  {
                     struct State
                     {
                        casual::cli::pipe::done::Scope done;
                        common::transaction::ID current;
                        std::vector< std::filesystem::path> files;

                        CASUAL_LOG_SERIALIZE(
                           CASUAL_SERIALIZE( done);
                           CASUAL_SERIALIZE( current);
                           CASUAL_SERIALIZE( files);
                        )
                     };

                     template< typename M>
                     auto consume_message( const State& state, const std::filesystem::path& path, const std::string& format)
                     {
                        common::Trace trace{ "file::manager::admin::cli::local::pipe::consume::detail::consume_message"};
                        common::log::debug( "path: ", path, " format: ", format);

                        basic_file_message< M> file_message;

                        std::ifstream input{ path};

                        auto archive = common::serialize::create::reader::strict::from( format, input);
                        archive >> file_message;

                        // forward the actual message downstream
                        casual::cli::pipe::forward::message( file_message.message);
                     }

                     auto deduce_format( const std::filesystem::path& path)
                     {
                        auto extension = path.extension();
                        if( ! extension.empty())
                           return extension.string().substr( 1); // skip the dot
                        
                        common::code::raise::error( common::code::casual::invalid_argument, "unable to deduce format from file extension: ", path);
                     }

                     common::message::Type deduce_message_type( const std::filesystem::path& path, const std::string& format)
                     {
                        // try open the file and deserialize the type info
                        std::ifstream input{ path};
                        
                        if( ! input)
                           common::code::raise::error( common::code::casual::invalid_file, "failed to open file for reading: ", path);

                        auto archive = common::serialize::create::reader::relaxed::from( format, input);

                        pipe::message_type deduction;
                        archive >> deduction;

                        return deduction.type;
                     }

                     auto consume_file( const State& state, const std::filesystem::path& path)
                     {
                        common::Trace trace{ "file::manager::admin::cli::local::pipe::consume::detail::consume_file"};
                        common::log::debug( "path: ", path);

                        auto format = deduce_format( path);
                        auto message_type = deduce_message_type( path, format);

                        auto file = pipe::reserve_file( state.current, path);

                        switch( message_type)
                        {
                           case common::message::Type::cli_queue_message:
                              consume_message< casual::cli::message::queue::Message>( state, file, format);
                              break;
                           case common::message::Type::cli_payload:
                              consume_message< casual::cli::message::payload::Message>( state, file, format);
                              break;
                           default:
                              common::code::raise::error( common::code::casual::invalid_argument, "file is not produced by casual: ", path);
                              break;
                        }

                        // remove the reserved file after consumptions
                        std::filesystem::remove( file);
                     }

                     auto consume_files( const State& state)
                     {
                        common::Trace trace{ "file::manager::admin::cli::local::pipe::consume::detail::consume_files"};

                        for( auto& file : state.files)
                           consume_file( state, file);
                     }

                     auto create_handle( detail::State& state)
                     {
                        // we're only interested in the first transaction::Current message to trigger
                        // our consume operation
                        return [ &state]( const casual::cli::message::transaction::Current& message)
                        {
                           common::Trace trace{ "file::manager::admin::cli::local::pipe::consume::detail::handle"};
                           common::log::debug( "message: ", message);
                           
                           if( ! state.current)
                           {
                              // the first transaction in the pipe becomes our current (could 
                              // be more transactions in the pipe, that we forward downstream)
                              state.current = message.trid;

                              consume_files( state);
                           }

                           // forward downstream
                           casual::cli::pipe::forward::message( message);
                        };
                     }

                     constexpr std::string_view description = R"(consumes provided glob(s) and sends them downpipe
                     
The files that glob(s) resolves to has to be produced by `casual file --produce`

Has to be used inside a pipe transaction.)";

                     constexpr std::string_view extended = R"(
Examples:

   `casual transaction --begin \
      | casual file --consume /path/to/files/* \
      | casual queue --enqueue my-queue \
      | casual transaction --commit`
   
   `casual transaction --begin \
      | casual file --consume "/path/to/files/*.json" \
      | casual call --service my-service \
      | casual transaction --commit`
)";
                     
                  } // detail
                  
                  auto option()
                  {
                     auto invoke = []( const std::vector< std::string>& globs)
                     {
                        common::Trace trace{ "file::manager::admin::cli::local::pipe::consume::invoke"};
                        common::log::debug( "glob: ", globs);

                        detail::State state;

                        state.files = common::file::find( globs);                        

                        auto handler = casual::cli::message::dispatch::create( 
                           casual::cli::pipe::forward::handle::defaults(),
                           std::ref( state.done),
                           detail::create_handle( state));

                        // start the pump
                        common::communication::stream::inbound::Device in{ std::cin};
                        common::message::dispatch::pump( casual::cli::pipe::condition::done( state.done), handler, in);

                        // state.done dtor will send Done downstream
                        
                     };

                     return argument::Option{
                        argument::option::one::many( std::move( invoke)),
                        {{ "--consume"}},
                        { detail::description, detail::extended}
                     };
                  }
                  
               } // consume
               
            } // pipe

         } // <unnamed>
      } // local


      argument::Option options()
      {
         return argument::Option
         { [](){}, {{ "file"}}, "file related administration"}
         ({
            local::list::reservations::option(),
            local::assets::recovery::option(),
            local::pipe::produce::option(),
            local::pipe::consume::option(),
            casual::cli::state::option(&local::detail::call::state),
         });
      }

   } // file::manager::admin::cli
} // casual
