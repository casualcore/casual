/*
 * field.h
 *
 *  Created on: Oct 8, 2017
 *      Author: kristone
 */

#ifndef MIDDLEWARE_BUFFER_INCLUDE_BUFFER_INTERNAL_FIELD_H_
#define MIDDLEWARE_BUFFER_INCLUDE_BUFFER_INTERNAL_FIELD_H_

#include <unordered_map>
#include <string>
#include <iosfwd>


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
            const std::unordered_map<std::string,int>& name_to_type();
            const std::unordered_map<int,std::string>& type_to_name();

            const std::unordered_map<std::string,long>& name_to_id();
            const std::unordered_map<long,std::string>& id_to_name();
            //! @}


            //!
            //! Export/Import
            //!
            //! @todo Better names
            //!
            //! @{
            std::ostream& dump( const char* buffer, std::ostream& stream, const std::string& protocol);
            //char* pump( std::istream& stream, const std::string& protocol);
            //!@}

         } // internal

      } // field

   } // buffer

} // casual




#endif /* MIDDLEWARE_BUFFER_INCLUDE_BUFFER_INTERNAL_FIELD_H_ */
