//!
//! casual_logger.h
//!
//! Created on: Jun 21, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_LOGGER_H_
#define CASUAL_LOGGER_H_


#include <string>


//
// TODO: Temp, we will not use iostream later on!
//
#include <sstream>

#include "casual_utility_platform.h"

namespace casual
{
	namespace logger
	{

		namespace internal
		{
			//
			// TODO: When common version of supported compilers have better support
			// for C++11, change the semantics of this class to only use move-semantics.
			//
			class Proxy
			{
			public:

				Proxy( int priority);

				//
				// We can't rely on RVO, so we have to release logging-responsibility for
				// rhs.
				//
				Proxy( const Proxy& rhs);

				//
				// Will be called when the full expression has "run", and this rvalue
				// will be destructed.
				//
				~Proxy();

				template< typename T>
				Proxy& operator << ( const T& value)
				{
					m_message << value;
					return *this;
				}

			private:
				//
				// TODO: We should use something else, hence not have any
				// dependencies to iostream
				//
				std::ostringstream m_message;
				int m_priority;
				mutable bool m_log;
			};

			template< int priority>
			class basic_logger
			{
			public:

				template< typename T>
				Proxy operator << ( const T& value)
				{
					return Proxy( priority) << value;
				}

			};

		} // internal



		extern internal::basic_logger< utility::platform::cLOG_debug> debug;

		extern internal::basic_logger< utility::platform::cLOG_warning> warning;

		extern internal::basic_logger< utility::platform::cLOG_error> error;



	} // logger

} // casual



#endif /* CASUAL_LOGGER_H_ */
