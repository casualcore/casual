//!
//! casual
//!

#ifndef CASUAL_COMMON_SERVER_ARGUMENTS_H_
#define CASUAL_COMMON_SERVER_ARGUMENTS_H_

#include "common/server/service.h"

#include "common/transaction/context.h"


#include <functional>
#include <vector>


extern "C"
{
extern int tpsvrinit( int argc, char** argv);
extern void tpsvrdone();
}

namespace casual
{
   namespace common
   {
      namespace server
      {

         struct Arguments
         {
            using function_init_type = std::function<int( int, char**)>;
            using function_done_type = std::function<void()>;

            Arguments( Arguments&&);
            Arguments& operator = (Arguments&&);
            Arguments( int argc, char** argv, function_init_type init, function_done_type done);
            Arguments( std::vector< std::string> args, function_init_type init, function_done_type done);

            std::vector< Service> services;

            int argc = 0;
            char** argv = nullptr;

            std::vector< std::string> arguments;

            function_init_type init;
            function_done_type done;

            std::vector< transaction::Resource> resources;

         private:
            std::vector< char*> c_arguments;
         };

      } // server
   } // common
} // casual

#endif // ARGUMENTS_H_
