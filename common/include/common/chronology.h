/*
 * chronology.h
 *
 *  Created on: 5 maj 2013
 *      Author: Kristone
 */

#ifndef CHRONOLOGY_H_
#define CHRONOLOGY_H_

#include "common/types.h"

#include <string>

namespace casual
{

namespace common
{

   namespace chronology
   {
      std::string local();
      std::string local( const common::time_type& time);
      std::string universal();
      std::string universal( const common::time_type& time);
   }

}

}


#endif /* CHRONOLOGY_H_ */
