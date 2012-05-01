//!
//! casual_broker.h
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_H_
#define CASUAL_BROKER_H_

#include <vector>
#include <string>

namespace casual
{

	class Broker
	{
	public:
		Broker( const std::vector< std::string>& arguments);
		~Broker();

		void start();

	};

}




#endif /* CASUAL_BROKER_H_ */
