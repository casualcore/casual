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
            struct Nested
            {
               template< typename T1, typename T2>
               struct Type
               {
                  //Type( T1&& t1, T2&& t2) : left( std::forward< T1>( t1)), right( std::forward< T2>( t2)) {}
                  Type( T1 t1, T2 t2) : left( t1), right( t2) {}

                  Type( Type&&) = default;

                  template< typename T>
                  auto operator () ( T&& value) const -> decltype( T1()( T2()( std::forward< T>( value))))
                  {
                     return left( right( std::forward< T>( value)));
                  }
               private:
                  T1 left;
                  T2 right;
               };

               template< typename T1, typename T2>
               static Type< T1, T2> make( T1&& t1, T2&& t2)
               {
                  return Type< T1, T2>( std::forward< T1>( t1), std::forward< T2>( t2));
               }
            };
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
