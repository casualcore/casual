//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/message/event.h"

#include "common/terminal.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace event
         {
            inline namespace v1 {
   
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
                           case Enum::success: return common::terminal::color::green;
                           case Enum::warning: return common::terminal::color::magenta;
                           case Enum::error: return common::terminal::color::red;
                        }
                        return common::terminal::color::red;
                     };
                  } // <unnamed>
               } // local
               std::ostream& print( std::ostream& out, const general::Task& event)
               {
                  out << common::terminal::color::blue << "task: ";
                  out << common::terminal::color::yellow << event.description << " - ";
                  return out << local::get_color( event.state) << event.state << '\n';
               }

               std::ostream& print( std::ostream& out, const general::sub::Task& event)
               {
                  out << common::terminal::color::blue << "sub task: ";
                  out << common::terminal::color::yellow << event.description << " - ";
                  return out << local::get_color( event.state) << event.state << '\n';
               }
            } // terminal
            
            } // inline v1
         } // event

      } // message
   } // common
} // casual