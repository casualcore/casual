/*
 * chronology.h
 *
 *  Created on: 5 maj 2013
 *      Author: kristian
 */

#ifndef CHRONOLOGY_H_
#define CHRONOLOGY_H_

#include "common/platform.h"

#include <string>

namespace casual
{

namespace common
{

   namespace chronology
   {
      std::string local();
      std::string local( const platform::time_type& time);
      std::string universal();
      std::string universal( const platform::time_type& time);
   }

}

}


#endif /* CHRONOLOGY_H_ */
