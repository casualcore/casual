//!
//! casual 
//!

#include "common/server/argument.h"

namespace casual
{
   namespace common
   {
      namespace server
      {
         Arguments::Arguments( Arguments&&) = default;
         Arguments& Arguments::operator = (Arguments&&) = default;

         Arguments::Arguments( int argc, char** argv)
            : argc( argc), argv( argv)
         {
         }

         Arguments::Arguments( std::vector< std::string> args)
            : arguments( std::move( args))
         {
            for( auto& argument : arguments)
            {
               c_arguments.push_back( const_cast< char*>( argument.c_str()));
            }
            argc = c_arguments.size();
            argv = c_arguments.data();
         }

      } // server
   } // common
} // casual
