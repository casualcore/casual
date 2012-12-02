//!
//! xatmi_server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#include "xatmi_server.h"


#include <vector>
#include <algorithm>


#include "common/service_context.h"
#include "common/server_context.h"


#include "utility/error.h"


namespace local
{
	namespace
	{
		struct AddServiceContext
		{
			void operator () ( const casual_service_name_mapping& mapping)
			{
				casual::common::service::Context context( mapping.m_name, mapping.m_functionPointer);

				casual::common::server::Context::instance().add( std::move( context));

			}
		};
	}

}


extern "C"
{
   int tpsvrinit(int argc, char **argv);

   void tpsvrdone();
}



int casual_startServer( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size)
{
   try
   {

      std::vector< std::string> arguments;

      std::copy(
         argv,
         argv + argc,
         std::back_inserter( arguments));


      std::for_each(
         mapping,
         mapping + size,
         local::AddServiceContext());

      tpsvrinit( argc, argv);

	   return casual::common::server::Context::instance().start();
	}
	catch( ...)
	{
	   tpsvrdone();
	   return casual::utility::error::handler();
	}
	return 0;
}








