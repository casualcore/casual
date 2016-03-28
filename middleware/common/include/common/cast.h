//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_CAST_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_CAST_H_


namespace casual
{
   namespace common
   {
      namespace cast
      {
         template< typename E>
         typename std::enable_if< std::is_enum< E>::value, typename std::underlying_type< E>::type>::type
         underlying( E value)
         {
            return static_cast< typename std::underlying_type< E>::type>( value);
         }


      } // cast


   } // common



} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_CAST_H_
