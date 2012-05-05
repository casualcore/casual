//!
//! casual_utility_file.h
//!
//! Created on: May 5, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_FILE_H_
#define CASUAL_UTILITY_FILE_H_

#include <string>


namespace casual
{

	namespace utility
	{
		namespace file
		{
			void remove( const std::string& path);

			class RemoveGuard
			{
			public:
				RemoveGuard( const std::string& path);
				~RemoveGuard();

				const std::string& path() const;

			private:
				RemoveGuard( const RemoveGuard&);
				RemoveGuard& operator = ( const RemoveGuard&);

				const std::string m_path;
			};

			class ScopedPath : public RemoveGuard
			{
			public:
				ScopedPath( const std::string& path);

				operator const std::string& ();
			};


		}

	}


}




#endif /* CASUAL_UTILITY_FILE_H_ */
