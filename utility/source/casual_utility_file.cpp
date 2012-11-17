//!
//! casual_utility_file.cpp
//!
//! Created on: May 5, 2012
//!     Author: Lazan
//!

#include "casual_utility_file.h"
#include <cstdio>


//TODO: temp
#include <iostream>

#include <dirent.h>

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


			std::string find( const std::string& path, const std::regex& search)
			{
			   std::string result;

			   DIR* directory = opendir( path.c_str());

			   if( directory)
			   {
			      struct dirent* element = nullptr;
			      while( ( element = readdir( directory)) != nullptr)
			      {
			         std::cout << "file: '" << element->d_name << "'" << std::endl;

			         if( std::regex_match( element->d_name, search))
			         {
			            result = path + "/";
			            result += element->d_name;
			            break;
			         }
			      }

			      closedir( directory);
			   }

			   return result;
			}

		}

	}


}

