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
            //Arguments() = default;
            Arguments( Arguments&&) = default;
            Arguments& operator = (Arguments&&) = default;

            Arguments( int argc, char** argv) : argc( argc), argv( argv)
            {

            }

            Arguments( std::vector< std::string> args) : arguments( std::move( args))
            {
               for( auto& argument : arguments)
               {
                  c_arguments.push_back( const_cast< char*>( argument.c_str()));
               }
               argc = c_arguments.size();
               argv = c_arguments.data();
            }

            std::vector< Service> services;
            std::function<int( int, char**)> server_init = &tpsvrinit;
            std::function<void()> server_done = &tpsvrdone;
            int argc;
            char** argv;

            std::vector< std::string> arguments;

            std::vector< transaction::Resource> resources;

         private:
            std::vector< char*> c_arguments;
         };



      } // server
   } // common


} // casual

#endif // ARGUMENTS_H_
