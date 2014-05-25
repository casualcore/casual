//!
//! move.h
//!
//! Created on: May 25, 2014
//!     Author: Lazan
//!

#ifndef MOVE_H_
#define MOVE_H_


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
         struct Active
         {
            Active() = default;

            Active( Active&& other) noexcept
            {
               other.m_active = false;
            }

            Active& operator = ( Active&& other) noexcept
            {
               other.m_active = false;
               return *this;
            }

            Active( const Active&) = delete;
            Active& operator = ( const Active&) = delete;


            explicit operator bool () { return m_active;}

         private:
            bool m_active = true;
         };




      } // move


   } // common


} // casual

#endif // MOVE_H_
