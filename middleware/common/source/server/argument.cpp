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

         Arguments::Arguments( int argc, char** argv, function_init_type init, function_done_type done)
            : argc( argc), argv( argv), init( std::move( init)), done( std::move( done))
         {
         }

         Arguments::Arguments( std::vector< std::string> args, function_init_type init, function_done_type done)
            : arguments( std::move( args)), init( std::move( init)), done( std::move( done))
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
