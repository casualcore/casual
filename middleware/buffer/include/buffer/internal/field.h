/*
 * field.h
 *
 *  Created on: Oct 8, 2017
 *      Author: kristone
 */

#ifndef MIDDLEWARE_BUFFER_INCLUDE_BUFFER_INTERNAL_FIELD_H_
#define MIDDLEWARE_BUFFER_INCLUDE_BUFFER_INTERNAL_FIELD_H_

#include <unordered_map>
#include <vector>
#include <string>


namespace casual
{
   namespace buffer
   {
      namespace field
      {
         namespace internal
         {
            //!
            //! Repository
            //!
            //! @{
            std::unordered_map<std::string,int> name_to_type();
            std::unordered_map<int,std::string> type_to_name();

            std::unordered_map<std::string,long> name_to_id();
            std::unordered_map<long,std::string> id_to_name();
            //! @}


            //!
            //! Export/Import
            //!
            //! @todo Better names
            //!
            //! @{

            //std::vector<std::pair<std::string,std::string>> dump( const char* buffer);
            //char* pump( const std::vector<std::pair<std::string,std::string>>& buffer);

            //!@}

         } // internal

      } // field

   } // buffer

} // casual




#endif /* MIDDLEWARE_BUFFER_INCLUDE_BUFFER_INTERNAL_FIELD_H_ */
