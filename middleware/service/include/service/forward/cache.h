//!
//! casual
//!

#ifndef CASUAL_SERVICE_FORWARD_CACHE_H_
#define CASUAL_SERVICE_FORWARD_CACHE_H_


#include "common/message/service.h"
#include "common/message/pending.h"

#include <map>
#include <vector>
#include <deque>
#include <string>


namespace casual
{
   namespace service
   {
      namespace forward
      {

         struct State
         {
            std::unordered_map< std::string, std::deque< common::message::service::call::callee::Request>> reqested;

            std::vector< common::message::pending::Message> pending;
         };

         struct Cache
         {
            Cache();
            ~Cache();

            void start();

         private:
            State m_state;

         };

      } // forward
   } // service
} // casual

#endif // CACHE_H_
