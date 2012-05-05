//!
//! casual_utility_file.cpp
//!
//! Created on: May 5, 2012
//!     Author: Lazan
//!

#include "casual_utility_file.h"
#include <cstdio>


namespace casual
{

	namespace utility
	{
		namespace file
		{
			void remove( const std::string& path)
			{
				std::remove( path.c_str());
			}

			RemoveGuard::RemoveGuard( const std::string& path)
				: m_path( path) {}

			RemoveGuard::~RemoveGuard()
			{
				remove( m_path);
			}

			const std::string& RemoveGuard::path() const
			{
				return m_path;
			}


			ScopedPath::ScopedPath( const std::string& path)
				: RemoveGuard( path) {}

			ScopedPath::operator const std::string& ()
			{
				return path();
			}


		}

	}


}

