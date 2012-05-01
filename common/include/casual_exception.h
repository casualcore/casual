//!
//! casual_exception.h
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_EXCEPTION_H_
#define CASUAL_EXCEPTION_H_


namespace casual
{
	namespace exception
	{

		struct MemoryNotFound : public std::exception {};

		struct EnvironmentVariableNotFound : public std::runtime_error
		{
			EnvironmentVariableNotFound( const std::string& description)
				: std::runtime_error( description) {}
		};

	}

}




#endif /* CASUAL_EXCEPTION_H_ */
