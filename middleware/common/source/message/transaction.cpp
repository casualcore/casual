//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/transaction.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace transaction
         {


            namespace rollback
            {
               std::ostream& operator << ( std::ostream& out, const Request& message)
               {
                  return out << "{ process: " << message.process
                        << ", trid: " << message.trid
                        << ", resources: " << range::make( message.resources)
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply::Stage& stage)
               {
                  switch( stage)
                  {
                     case Reply::Stage::rollback: return out << "rollback";
                     case Reply::Stage::error: return out << "error";
                  }
                  return out << "unknown";
               }
               std::ostream& operator << ( std::ostream& out, const Reply& message)
               {
                  return out << "{ process: " << message.process
                        << ", trid: " << message.trid
                        << ", stage: " << message.stage
                        << ", state: " << message.state
                        << '}';
               }

            } // rollback

            namespace commit
            {
               std::ostream& operator << ( std::ostream& out, const Reply::Stage& stage)
               {
                  switch( stage)
                  {
                     case Reply::Stage::commit: return out << "commit";
                     case Reply::Stage::error: return out << "error";
                     case Reply::Stage::prepare: return out << "prepare";
                  }
                  return out << "unknown";
               }
               std::ostream& operator << ( std::ostream& out, const Reply& message)
               {
                  return out << "{ process: " << message.process
                        << ", trid: " << message.trid
                        << ", state: " << message.stage
                        << ", state: " << message.state
                        << '}';
               }
               std::ostream& operator << ( std::ostream& out, const Request& message)
               {
                  return out << "{ process: " << message.process
                        << ", trid: " << message.trid
                        << ", resources: " << range::make( message.resources)
                        << '}';
               }
            } // commit

            namespace resource
            {

               std::ostream& operator << ( std::ostream& out, const Involved& value)
               {
                  return out << "{ process: " << value.process
                        << ", resources: " << range::make( value.resources)
                        << ", trid: " << value.trid
                        << '}';
               }

               namespace external
               {
                  std::ostream& operator << ( std::ostream& out, const Involved& value)
                  {
                     return out << "{ process: " << value.process
                           << ", trid: " << value.trid
                           << '}';
                  }

               } // domain


               namespace connect
               {

                  std::ostream& operator << ( std::ostream& out, const Reply& message)
                  {
                     return out << "{ process: " << message.process << ", resource: " << message.resource << ", state: " << message.state << "}";
                  }

               } // connect

            } // resource
         } // transaction
      } // message
   } // common
} // casual

