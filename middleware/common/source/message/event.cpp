//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/message/event.h"

#include "common/terminal.h"
#include "common/log/stream.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace event
         {
            inline namespace v1 {

            namespace local
            {
               namespace
               {
                  namespace sub
                  {
                     constexpr auto indentation = "  ";
                  } // subtask
               } // <unnamed>
            } // local
            
            std::ostream& operator << ( std::ostream& out, Error::Severity value)
            {
               switch( value)
               {
                  using Enum = Error::Severity;
                  case Enum::fatal: return out << "fatal";
                  case Enum::error: return out << "error";
               }
               return out << "<unknown>";
            }

            namespace task
            {
               std::ostream& operator << ( std::ostream& out, State state)
               {
                  switch( state)
                  {
                     using Enum = decltype( state);
                     case Enum::started: return out << "started";
                     case Enum::done: return out << "done";
                     case Enum::aborted: return out << "aborted";
                     case Enum::warning: return out << "warning";
                     case Enum::error: return out << "error";
                  }
                  return out << "<unknown>";
               }
            } // task

   
            namespace terminal
            {
               namespace local
               {
                  namespace
                  {
                     auto get_color = []( auto state)
                     {
                        switch( state)
                        {
                           using Enum = decltype( state);
                           case Enum::started: return common::terminal::color::green;
                           case Enum::done: return common::terminal::color::green;
                           case Enum::aborted: return common::terminal::color::magenta;
                           case Enum::warning: return common::terminal::color::magenta;
                           case Enum::error: return common::terminal::color::red;
                        }
                        return common::terminal::color::red;
                     };
                  } // <unnamed>
               } // local

               std::ostream& print( std::ostream& out, const Error& event)
               {
                  auto severity_color = [&event]()
                  {
                     switch( event.severity)
                     {
                        using Enum = decltype( event.severity);
                        case Enum::fatal: return common::terminal::color::value::red;
                        case Enum::error: return common::terminal::color::value::red;
                     }
                     return common::terminal::color::value::red;
                  };

                  common::stream::write( out, event::local::sub::indentation, severity_color(), event.severity,  ' ', event.code, ": ", common::terminal::color::value::no_color);
                  return  out << common::terminal::color::white << event.message << '\n'; 
               }

               std::ostream& print( std::ostream& out, const Notification& event)
               {
                  return common::stream::write( out, common::terminal::color::value::cyan, "information: ", common::terminal::color::value::white, event.message, common::terminal::color::value::no_color, '\n');
               }

               std::ostream& print( std::ostream& out, const process::Spawn& event)
               {
                  out << event::local::sub::indentation << common::terminal::color::blue << "alias spawn: ";
                  out << common::terminal::color::yellow << event.alias << " ";
                  return out << common::terminal::color::no_color << range::make( event.pids) << '\n'; 
              }

               std::ostream& print( std::ostream& out, const process::Exit& event)
               {  
                  out << event::local::sub::indentation << common::terminal::color::blue << "process exit: ";
                  
                  auto exit_color = []( auto status){ return status == 0 ? common::terminal::color::green : common::terminal::color::magenta;};

                  switch( event.state.reason)
                  {
                     using Enum = decltype( event.state.reason);
                     
                     case Enum::core:
                        out << common::terminal::color::red << event.state.reason << " ";
                        return out << common::terminal::color::white << event.state.pid << '\n';
                     case Enum::exited:
                        out << common::terminal::color::green << event.state.reason << " "; 
                        out << common::terminal::color::white << event.state.pid;
                        return out << " - status: " << exit_color( event.state.status) << event.state.status << '\n';
                     default: 
                        out << common::terminal::color::magenta << event.state.reason << " "; 
                        return out << common::terminal::color::white << event.state.pid << '\n';
                  }
               }

               std::ostream& print( std::ostream& out, const Task& event)
               {
                  out << common::terminal::color::blue << "task: ";
                  out << common::terminal::color::yellow << event.description << " - ";
                  return out << local::get_color( event.state) << event.state << '\n';
               }

               std::ostream& print( std::ostream& out, const sub::Task& event)
               {
                  out << event::local::sub::indentation << common::terminal::color::blue << "sub task: ";
                  out << common::terminal::color::yellow << event.description << " - ";
                  return out << local::get_color( event.state) << event.state << '\n';
               }
            } // terminal
            
            } // inline v1
         } // event

      } // message
   } // common
} // casual