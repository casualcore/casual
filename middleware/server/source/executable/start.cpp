//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "server/executable/start.h"

#include "common/log.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include "transaction/context.h"



namespace casual
{
   namespace server::executable
   {
      inline namespace v1
      {
         int start( std::vector< argument::transaction::Resource> resources, common::unique_function< int()> user_main)
         {
            common::Trace trace{ "server::executable::start"};

            // validate that all resources are named
            if( common::algorithm::any_of( resources, [](auto& r){ return r.name.empty();}))
               common::code::raise::error( common::code::casual::invalid_semantics, "casual resource executable has to be built with named resources");

            // configure resources
            casual::transaction::context().configure( std::move( resources));

            return user_main();
         }

      } // v1

   } // server::executable
} // casual
