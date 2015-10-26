//!
//! pimpl.h
//!
//! Created on: Dec 21, 2013
//!     Author: Lazan
//!

#ifndef PIMPL_H_
#define PIMPL_H_


#include <memory>

namespace casual
{

   namespace sf
   {
      template< typename T>
      class Pimpl
      {
      public:

         using implementation_type = T;

         //!
         //!
         //! @note change to std::make_uniqeu
         //!
         template< typename ...Args>
         Pimpl( Args&&... args) : m_holder( new implementation_type{ std::forward< Args>( args)...}) {}


         //!
         //!
         //! @note change to std::make_uniqeu
         //!
         Pimpl( const Pimpl& other) : m_holder( new implementation_type( other.implementation())) {}

         Pimpl& operator = ( const Pimpl& other)
         {
            Pimpl temporary{ other};
            m_holder = std::move( temporary.m_holder);
            return *this;
         }

         Pimpl( Pimpl&&) noexcept = default;
         Pimpl& operator = ( Pimpl&&) noexcept = default;

         ~Pimpl() = default;

         implementation_type& implementation() { return *m_holder;}
         implementation_type& implementation() const { return *m_holder;}



      private:
         std::unique_ptr< implementation_type> m_holder;

      };

   } // sf


} // casual

#endif // PIMPL_H_
