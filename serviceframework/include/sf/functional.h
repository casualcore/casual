//!
//! functional.h
//!
//! Created on: Jul 12, 2013
//!     Author: Lazan
//!

#ifndef FUNCTIONAL_H_
#define FUNCTIONAL_H_


namespace casual
{
   namespace sf
   {
      namespace functional
      {
         namespace link
         {
            template< typename L>
            struct basic_link
            {
               template< typename T1, typename T2>
               struct Type
               {
                  typedef L link_type;

                  Type( T1 t1, T2 t2) : left( t1), right( t2) {}
                  Type( Type&&) = default;
                  Type( const Type&) = default;

                  template< typename T>
                  auto operator () ( T&& value) const -> decltype( link_type::link( std::declval< T1>(), std::declval< T2>(), std::forward< T>( value)))
                  {
                     return link_type::link( left, right, std::forward< T>( value));
                  }

               protected:
                  T1 left;
                  T2 right;
               };

               template< typename T1, typename T2>
               static Type< T1, T2> make( T1&& t1, T2&& t2)
               {
                  return Type< T1, T2>( std::forward< T1>( t1), std::forward< T2>( t2));
               }
            };

            namespace detail
            {
               struct Nested
               {
                  template< typename T1, typename T2, typename T>
                  static auto link( T1 left, T2 right, T&& value) -> decltype( left( right( std::forward< T>( value))))
                  {
                     return left( right( std::forward< T>( value)));
                  }
               };

               struct And
               {
                  template< typename T1, typename T2, typename T>
                  static auto link( T1 left, T2 right, T&& value) -> decltype( left( value) && right( value))
                  {
                     return left( value) && right( value);
                  }
               };

               struct Or
               {
                  template< typename T1, typename T2, typename T>
                  static auto link( T1 left, T2 right, T&& value) -> decltype( left( value) || right( value))
                  {
                     return left( value) || right( value);
                  }
               };

            } // detail

            using Nested = basic_link< detail::Nested>;

            using And = basic_link< detail::And>;

            using Or = basic_link< detail::Or>;

         } // link

         template< typename Link>
         struct Chain
         {
            template< typename Arg>
            static auto link( Arg&& param) -> decltype( std::forward< Arg>( param))
            {
               return std::forward< Arg>( param);
            }

            template< typename Arg, typename... Args>
            static auto link( Arg&& param, Args&&... params) -> decltype( Link::make( std::forward< Arg>( param), link( std::forward< Args>( params)...)))
            {
               return Link::make( std::forward< Arg>( param), link( std::forward< Args>( params)...));
            }
         };


         namespace extract
         {
            struct Second
            {
               template< typename T>
               auto operator () ( T&& value) const -> decltype( value.second)
               {
                  return value.second;
               }
            };

         }
      } // functional
   } // sf
} // casual



#endif /* FUNCTIONAL_H_ */
