//!
//! casual
//!

#ifndef CASUAL_COMMON_NETWORK_BYTEORDER_H_
#define CASUAL_COMMON_NETWORK_BYTEORDER_H_

#include <type_traits>
#include <cstdint>

namespace casual
{
   namespace common
   {
      namespace network
      {

         namespace byteorder
         {

            namespace detail
            {

               template< int size, bool is_signed, bool is_float>
               struct transcode;

               //!
               //! 1 byte representation, inline.
               //! @{
               template<>
               struct transcode< 1, false, false>
               {
                  using value_type = std::uint8_t;
                  inline static std::uint8_t encode( value_type value) noexcept { return value;}
                  inline static std::uint8_t decode( value_type value) noexcept { return value;}
               };

               template<>
               struct transcode< 1, true, false> : transcode< 1, false, false> {};
               //! @}

               //!
               //! 2 bytes representation
               //! @{
               template<>
               struct transcode< 2, false, false>
               {
                  using value_type = std::uint16_t;
                  static std::uint16_t encode( value_type value) noexcept;
                  static std::uint16_t decode( value_type value) noexcept;
               };

               template<>
               struct transcode< 2, true, false> : transcode< 2, false, false> {};
               //! @}


               //!
               //! 4 bytes representation, we're not using this right now...
               //! @{
               template<>
               struct transcode< 4, false, false>
               {
                  using value_type = std::uint32_t;
                  static std::uint32_t encode( value_type value) noexcept;
                  static value_type decode( std::uint32_t value) noexcept;
               };

               template<>
               struct transcode< 4, true, false> : transcode< 4, false, false> {};
               //! @}


               //!
               //! 8 bytes representation
               //! @{
               template<>
               struct transcode< 8, false, false>
               {
                  using value_type = std::uint64_t;
                  static std::uint64_t encode( value_type value) noexcept;
                  static std::uint64_t decode( std::uint64_t value) noexcept;
               };

               template<>
               struct transcode< 8, true, false> : transcode< 8, false, false> {};
               //! @}





               //!
               //! float representation
               //! @{

               template<>
               struct transcode< 8, true, true>
               {
                  using value_type = double;
                  static std::uint64_t encode( value_type value) noexcept;
                  static value_type decode( std::uint64_t value) noexcept;
               };

               template<>
               struct transcode< 4, true, true>
               {
                  using value_type = float;
                  static std::uint32_t encode( value_type value) noexcept;
                  static value_type decode( std::uint32_t value) noexcept;
               };

               //! @}

               namespace size
               {
                  //!
                  //! Defines the serialized size of a native type.
                  //! also implicit defines which types we handle.
                  //!
                  template< typename T>
                  struct traits;

                  template<> struct traits< bool>{ enum{ size = 1};};

                  template<> struct traits< signed char>{ enum{ size = 1};};
                  template<> struct traits< char>{ enum{ size = 1};};
                  template<> struct traits< unsigned char>{ enum{ size = 1};};

                  template<> struct traits< signed short>{ enum{ size = 2};};
                  template<> struct traits< unsigned short>{ enum{ size = 2};};

                  template<> struct traits< unsigned int>{ enum{ size = 8};};
                  template<> struct traits< signed int>{ enum{ size = 8};};

                  template<> struct traits< unsigned long>{ enum{ size = 8};};
                  template<> struct traits< signed long>{ enum{ size = 8};};

                  template<> struct traits< unsigned long long>{ enum{ size = 8};};
                  template<> struct traits< signed long long>{ enum{ size = 8};};

                  template<> struct traits< float>{ enum{ size = 4};};
                  template<> struct traits< double>{ enum{ size = 8};};

               } // size

               template< typename T>
               auto encode( T value) noexcept -> decltype(
                  transcode< size::traits< T>::size, std::is_signed< T>::value, std::is_floating_point< T>::value>::encode(
                        static_cast< typename transcode< size::traits< T>::size, std::is_signed< T>::value, std::is_floating_point< T>::value>::value_type>( value)))
               {
                  using transcode_type = transcode< size::traits< T>::size, std::is_signed< T>::value, std::is_floating_point< T>::value>;

                  return transcode_type::encode( static_cast< typename transcode_type::value_type>( value));
               }


               template< typename R, typename T>
               R decode( T value) noexcept
               {
                  using transcode_type = transcode< size::traits< R>::size, std::is_signed< R>::value, std::is_floating_point< R>::value>;

                  return static_cast< R>( transcode_type::decode( value));
               }

            } // detail


            template< typename T>
            auto encode( T value) noexcept -> decltype( detail::encode( value))
            {
               return detail::encode( value);
            }

            template< typename R, typename T>
            auto decode( T value) noexcept -> decltype( detail::decode< R>( value))
            {
               return detail::decode< R>( value);
            }


            template<typename T>
            using type = decltype( encode( T()));

            template< typename T>
            constexpr std::size_t bytes() noexcept
            {
               return sizeof( decltype( encode( T())));
            }

         } // byteorder

      }
   }
}

#endif /* CASUAL_COMMON_NETWORK_BYTEORDER_H_ */
