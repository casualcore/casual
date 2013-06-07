//!
//! casual_utility_flag.h
//!
//! Created on: Jul 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_FLAG_H_
#define CASUAL_UTILITY_FLAG_H_



namespace casual
{
	namespace common
	{

		template< std::size_t flags, typename T>
		constexpr inline bool flag( T value)
		{
			return ( value & flags) == flags;
		}



	}

}


#endif /* CASUAL_UTILITY_FLAG_H_ */
