//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/manager/admin/cli.h"

#include "file/manager/admin/model.h"
#include "file/manager/admin/services.h"

#include "common/log/line.h"

#include "common/terminal.h"

#include "casual/cli/state.h"

#include "serviceframework/service/protocol/call.h"

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
                  using Call = serviceframework::service::protocol::binary::Call;

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
                     return common::terminal::format::formatter< manager::admin::model::Request>::construct(
                        common::terminal::format::column( "path", []( const auto& r){ return r.path;}, common::terminal::color::yellow),
                        common::terminal::format::column( "pid", []( const auto& r){ return r.pid;}, common::terminal::color::white),
                        common::terminal::format::column( "gtrid", []( const auto& r){ return r.gtrid;}),
                        common::terminal::format::column( "stage", []( const auto& r){ return description( r.stage);}),
                        common::terminal::format::column( "time", []( const auto& r){ return std::chrono::floor< std::chrono::microseconds>( r.time);}, common::terminal::color::blue));
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

                        auto formatter = detail::format::requests( state);

                        formatter.print( std::cout, state.requests);
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "-r", "--list-reservations"},
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
                           { "--recover-transactions-commit"},
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
                           { "--recover-transactions-rollback"},
                           "recover global transactions with rollback"
                        };
                     }
                  } // rollback
               } // recovery

            } // assets

         } //
      } // local


      argument::Option options()
      {
         return argument::Option
         { [](){}, { "file"}, "file related administration"}
         ({
            casual::cli::state::option(&local::detail::call::state),
            local::list::reservations::option(),
            local::assets::recovery::commit::option(),
            local::assets::recovery::rollback::option(),
         });
      }

   } // file::manager::admin::cli
} // casual