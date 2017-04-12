//!
//! casual 
//!

#include "common/message/event.h"


namespace casual
{

   namespace common
   {
      namespace message
      {
         namespace event
         {
            namespace subscription
            {


               std::ostream& operator << ( std::ostream& out, const Begin& value)
               {
                  return out << "{ process: " << value.process
                        << ", types: " << range::make( value.types)
                        << '}';
               }


               std::ostream& operator << ( std::ostream& out, const End& value)
               {
                  return out << "{ process: " << value.process
                        << '}';
               }


            } // subscription

            namespace process
            {
               std::ostream& operator << ( std::ostream& out, const Spawn& value)
               {
                  return out << "{ path: " << value.path
                        << ", pids: " << range::make( value.pids)
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Exit& value)
               {
                  return out << "{ state: " << value.state
                        << '}';
               }

            } // process
         } // event
      } // message
   } // common
} // casual
