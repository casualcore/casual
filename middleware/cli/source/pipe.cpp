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
   namespace cli::pipe
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

      namespace done
      {

         void Detector::state( State state)
         {
            // we only update if the new state is more _severe_
            if( m_state < state)
               m_state = state;
         }

         State Detector::state() const noexcept
         {
            return m_state;
         }

         bool Detector::pipe_error() const noexcept
         {
            return m_state != State::ok;
         }

         Detector::operator bool() const noexcept
         {
            if( m_done || cli::pipe::terminal::in() || std::cin.peek() == std::istream::traits_type::eof())
            {
               common::log::line( verbose::log, "cli::pipe::condition::done - is done");
               return true;
            }
            return false;
         }

         void Detector::operator () ( const cli::message::pipe::Done& message)
         {
            m_done = true;
            state( message.state);
         }


         Scope::Scope() : m_uncaught_count{ std::uncaught_exceptions()}
         {}    
         
         Scope::~Scope()
         {
            common::exception::guard( [ &]()
            {
               // if the out is a terminal, we don't pipe it.
               if( pipe::terminal::out())
                  return;
               
               // if we're in stack unwinding -> always error
               if( m_uncaught_count != std::uncaught_exceptions())
                  state( State::error);

               cli::message::pipe::Done message;
               message.state = state();
               pipe::forward::message( message);
            });
         }
   
      } // done

   } // cli::pipe
} // casual