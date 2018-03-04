//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_MOVE_H_
#define CASUAL_COMMON_MOVE_H_

namespace casual
{
   namespace common
   {
      namespace move
      {

         //!
         //! indicator type to deduce if it has been moved or not.
         //!
         //! usecase:
         //!  Active as an attribute in <some type>
         //!  <some type> can use default move ctor and move assignment.
         //!  use Active attribute in dtor do deduce if instance of <some type> still
         //!  has responsibility...
         //!
         struct Moved
         {
            Moved() = default;

            Moved( Moved&& other) noexcept
            {
               other.m_moved = true;
            }

            Moved& operator = ( Moved&& other) noexcept
            {
               other.m_moved = true;
               return *this;
            }

            Moved( const Moved&) = delete;
            Moved& operator = ( const Moved&) = delete;


            explicit operator bool () const noexcept { return m_moved;}

            void release() noexcept { m_moved = true;}

         private:
            bool m_moved = false;
         };
      } // move
   } // common
} // casual

#endif // MOVE_H_
