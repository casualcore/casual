//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_BUFFER_FUNCTIONAL_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_BUFFER_FUNCTIONAL_H_


namespace casual
{
   namespace common
   {

      namespace details
      {

         //!
         //! memberfunction
         //!
         template< typename Base, typename R, typename Derived, typename... Args>
         decltype( auto) invoke( R Base::*pmf, Derived&& derived, Args&&... args)
         {
            return (std::forward<Derived>( derived).*pmf)(std::forward<Args>(args)...);
         }

         //!
         //! free function/functor
         //!
         template< typename F, typename... Args>
         decltype( auto) invoke( F&& function, Args&&... args)
         {
            return std::forward<F>( function)(std::forward<Args>(args)...);
         }

      } // details


      template< typename F, typename... Args>
      decltype( auto) invoke( F&& function, Args&&... args)
      {
         return details::invoke( std::forward< F>( function), std::forward<Args>(args)...);
      }


   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_BUFFER_FUNCTIONAL_H_
