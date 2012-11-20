//!
//! casual_service_context.h
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVICE_CONTEXT_H_
#define CASUAL_SERVICE_CONTEXT_H_

#include "xatmi.h"

#include <cstddef>

#include <string>

namespace casual
{
	namespace service
	{

		struct Context
		{
			Context( const std::string& name, tpservice function);

			Context();


			void call( TPSVCINFO* serviceInformation);

			std::string m_name;
			tpservice m_function;
			std::size_t m_called;


		};






	}


}


#endif /* CASUAL_SERVICE_CONTEXT_H_ */
