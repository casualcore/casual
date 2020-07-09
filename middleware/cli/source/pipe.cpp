//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/cli/pipe.h"
#include "casual/cli/common.h"

#include "common/communication/ipc.h"

#include <iostream>

#include <unistd.h>
#include <stdio.h>

namespace casual
{
   using namespace common;
   namespace cli
   {
      namespace pipe
      {
         namespace terminal
         {
            namespace local
            {
               namespace
               {
                  auto is_terminal( int fd)
                  {
                     return ::isatty( fd) == 1;
                  }
               } // <unnamed>
            } // local
            bool out()
            {
               return local::is_terminal( ::fileno( stdout));
            }
          
            bool in()
            {
               return local::is_terminal( ::fileno( stdin));
            }

         } // terminal
         namespace forward
         {
            namespace standard
            {
               void in()
               {
                  Trace trace{ "cli::pipe::forward::standard::in"};

                  if( ! cli::pipe::terminal::in() && std::cin.peek() != std::istream::traits_type::eof())
                     std::cout << std::cin.rdbuf();
               }
            } // standard
            
         } // forward

         void done()
         {
             Trace trace{ "cli::pipe::done"};

            if( ! pipe::terminal::out())
               forward::message( cli::message::pipe::Done{});

            std::cout.flush();
         }

         namespace transaction
         {
            void Association::operator() ( common::transaction::ID& trid)
            {
               // the trid might be associated already
               if( trid)
                  return;

               // associate based on the transaction-context
               switch( directive.context())
               {
                  using Context = decltype( directive.context());

                  case Context::absent:
                     // no association present
                     return;

                  case Context::single:
                     trid = directive.transaction.trid;
                     break;
                  case Context::compound:
                     trid = common::transaction::id::create( directive.process);
                     common::log::line( verbose::log, "created trid: ", trid);
                     break;
               }


               // we might have associated this trid before
               if( algorithm::find( associated, trid))
                  return;

               associated.push_back( trid);

               // we might be the 'owner', hence know about the transaction
               if( trid.owner() == common::process::handle())
                  return;

               // make sure the owner knows that we've associated a new trid
               cli::message::transaction::Associated message{ common::process::handle()};
               message.trid = trid;
               common::communication::device::blocking::send( directive.process.ipc, message);
            }


            namespace handle
            {
               common::function< void(cli::message::transaction::Directive&)> directive( Association& associator)
               {
                  return [&]( cli::message::transaction::Directive& message)
                  {
                     if( associator && associator.directive.process != common::process::handle())
                     {
                        // we need to send 'termination' to owner
                        common::communication::device::blocking::send( 
                           associator.directive.process.ipc, 
                           cli::message::transaction::directive::Terminated{ common::process::handle()});
                     }

                     associator.directive = std::move( message);
                  };
               }
               
            } // handle

         } // transaction

      } // pipe
   } // cli
} // casual