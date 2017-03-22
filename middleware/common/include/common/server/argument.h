//!
//! casual
//!

#ifndef CASUAL_COMMON_SERVER_ARGUMENTS_H_
#define CASUAL_COMMON_SERVER_ARGUMENTS_H_

#include "common/server/service.h"

#include "common/transaction/context.h"


#include <functional>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace server
      {

         struct Arguments
         {
            Arguments( Arguments&&);
            Arguments& operator = (Arguments&&);
            Arguments( int argc, char** argv);
            Arguments( std::vector< std::string> args);

            std::vector< Service> services;

            std::function<int( int, char**)> server_init;
            std::function<void()> server_done;

            int argc = 0;
            char** argv = nullptr;

            std::vector< std::string> arguments;

            std::vector< transaction::Resource> resources;

         private:
            std::vector< char*> c_arguments;
         };

      } // server
   } // common
} // casual

#endif // ARGUMENTS_H_
