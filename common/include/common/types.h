//!
//! types.h
//!
//! Created on: Nov 24, 2012
//!     Author: Lazan
//!

#ifndef TYPES_H_
#define TYPES_H_

#include <vector>
#include <string>
#include <tuple>
#include <chrono>

#include <string>

#include <xa.h>


namespace casual
{
   namespace common
   {
      typedef std::vector< char> binary_type;

      typedef char* raw_buffer_type;

      // TODO: change to: typedef std::chrono::steady_clock clock_type;
      // When clang has to_time_t for steady_clock
      typedef std::chrono::system_clock clock_type;


      typedef clock_type::time_point time_type;


      namespace transform
      {
         inline char* public_buffer( raw_buffer_type buffer)
         {
            return const_cast< char*>( buffer);
         }

         inline std::string time( const common::time_type& timepoint)
         {
            std::time_t tt;
            tt = clock_type::to_time_t ( timepoint);
            return std::ctime( &tt);
         }

         inline common::time_type time(long long value)
         {
            common::time_type::rep representation = value;
            return common::time_type( time_type::duration( representation));
         }

         /*
         inline std::tuple< binary_type, binary_type> xid( const XID& value)
         {
            return std::tuple< binary_type, binary_type>{
               {  value.data, value.data + value.gtrid_length },
               {  value.data + value.gtrid_length, value.data + value.gtrid_length + value.bqual_length}};
         }
         */



      } // transform
   } // common


   //!
   //! Overload for time_type
   //!
   //! @{
   template< typename M>
   void casual_marshal_value( casual::common::time_type& value, M& marshler)
   {
      auto time = value.time_since_epoch().count();
      marshler << time;
   }

   template< typename M>
   void casual_unmarshal_value( casual::common::time_type& value, M& unmarshler)
   {
      casual::common::time_type::rep representation;
      unmarshler >> representation;
      value = casual::common::time_type( casual::common::time_type::duration( representation));
   }
   //! @}


} // casual




#endif /* TYPES_H_ */
