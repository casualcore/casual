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

#include <xa.h>


namespace casual
{
   namespace common
   {
      typedef std::vector< char> binary_type;

      typedef char* raw_buffer_type;
      typedef const char* const_raw_buffer_type;

      // TODO: change to: typedef std::chrono::steady_clock clock_type;
      // When clang has to_time_t for steady_clock
      typedef std::chrono::system_clock clock_type;


      typedef clock_type::time_point time_type;


      enum
      {
         cNull_XID = -1
      };

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

         inline std::tuple< binary_type, binary_type> xid( const XID& value)
         {
            return std::tuple< binary_type, binary_type>{
               {  value.data, value.data + value.gtrid_length },
               {  value.data + value.gtrid_length, value.data + value.gtrid_length + value.bqual_length}};
         }


      }
   }

   //!
   //! Overload for time_type
   //!
   //! @{
   template< typename M>
   void marshal_value( M& marshler, common::time_type& value)
   {
      auto time = value.time_since_epoch().count();
      marshler << time;
   }

   template< typename M>
   void unmarshal_value( M& unmarshler, common::time_type& value)
   {
      common::time_type::rep representation;
      unmarshler >> representation;
      value = common::time_type( common::time_type::duration( representation));
   }
   //! @}

   //!
   //! Overload for XID
   //!
   //! @{
   template< typename M>
   void marshal_value( M& marshler, const XID& value)
   {
      marshler << value.formatID;

      if( value.formatID != common::cNull_XID)
      {

         marshler << value.gtrid_length;
         marshler << value.bqual_length;

         const auto size = marshler.m_buffer.size();
         marshler.m_buffer.resize( size + value.gtrid_length + value.bqual_length);
         std::copy(
             std::begin( value.data),
             std::begin( value.data) + value.gtrid_length + value.bqual_length,
             &marshler.m_buffer[ size]);
      }
   }

   template< typename M>
   void unmarshal_value( M& unmarshler, XID& value)
   {
      unmarshler >> value.formatID;

      if( value.formatID != common::cNull_XID)
      {
         unmarshler >> value.gtrid_length;
         unmarshler >> value.bqual_length;

         auto start = std::begin( unmarshler.m_buffer) + unmarshler.m_offset;
         auto end = start + value.gtrid_length + value.bqual_length;

         std::copy(
               start,
               end,
               value.data);
      }
   }
   //! @}


   inline bool is_null( const XID& xid)
   {
      return xid.formatID == common::cNull_XID;
   }

} // casual

inline bool operator < ( const XID& lhs, const XID& rhs)
{
   return std::lexicographical_compare(
         std::begin( lhs.data), std::begin( lhs.data) + lhs.gtrid_length + lhs.bqual_length,
         std::begin( rhs.data), std::begin( rhs.data) + rhs.gtrid_length + rhs.bqual_length);
}

inline bool operator == ( const XID& lhs, const XID& rhs)
{
   // TODO: will work in C++14
   /*
   return std::equal(
         std::begin( lhs.data), std::begin( lhs.data) + lhs.gtrid_length + lhs.bqual_length,
         std::begin( rhs.data), std::begin( rhs.data) + rhs.gtrid_length + rhs.bqual_length);
   */
   return lhs.gtrid_length + lhs.bqual_length == rhs.gtrid_length + rhs.bqual_length &&
         std::equal(
           std::begin( lhs.data), std::begin( lhs.data) + lhs.gtrid_length + lhs.bqual_length,
           std::begin( rhs.data));
}

inline bool operator == ( const XID& xid, std::nullptr_t)
{
   return xid.formatID == casual::common::cNull_XID;
}

inline bool operator == ( std::nullptr_t, const XID& xid)
{
   return xid.formatID == casual::common::cNull_XID;
}




#endif /* TYPES_H_ */
