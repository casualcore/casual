//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_COMMON_H_
#define CASUAL_COMMON_EXCEPTION_COMMON_H_

#include <system_error>

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace detail 
         {
            template< typename E>
            auto make_error_code( E code) { return code;}

            inline auto make_error_code( std::errc code) { return std::make_error_code( code);}
         } // detail 

         using base = std::system_error;

         template< typename Enum>
         class base_error : public base
         {
         public:
            base_error( Enum error) : base{ detail::make_error_code( error)} {}
            base_error( Enum error, const std::string& message) : base{ detail::make_error_code( error), message} {}
            base_error( Enum error, const char* message) : base{ detail::make_error_code( error), message} {}

            Enum type() const noexcept { return static_cast< Enum>( base::code().value());}

         };

         namespace detail
         {
            template< typename Base>
            using enum_type = decltype( std::declval< Base>().type());
         } // detail

         template< typename Base, detail::enum_type< Base> error>
         class basic_error : public Base
         {
         public:
            using base_type = Base;
            basic_error() : base_type{ error} {}
            basic_error( const std::string& message) : base_type{ error, message} {}
            basic_error( const char* message) : base_type{ error, message} {}

            constexpr static auto type() noexcept { return error;}
         };

      } // exception 
   } // common
} // casual

namespace std
{
   inline ostream& operator << ( ostream& out, const system_error& value)
   {
      return out << value.code() << ' ' << value.what();
   }

   inline ostream& operator << ( ostream& out, const exception& value)
   {
      return out << value.what();
   }
} // std

#endif
