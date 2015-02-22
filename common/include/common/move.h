//!
//! move.h
//!
//! Created on: May 25, 2014
//!     Author: Lazan
//!

#ifndef MOVE_H_
#define MOVE_H_

#include <memory>

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



         template< typename T>
         class basic_pimpl
         {
         public:

            using implementation_type = T;

            //!
            //!
            //! @note change to std::make_uniqeu
            //!
            template< typename ...Args>
            basic_pimpl( Args&&... args) : m_holder( new implementation_type{ std::forward< Args>( args)...}) {}

            basic_pimpl() : m_holder( new implementation_type{}) {}


            basic_pimpl( basic_pimpl&&) noexcept = default;
            basic_pimpl& operator = ( basic_pimpl&&) noexcept = default;

            ~basic_pimpl() = default;

            implementation_type* operator -> () const { return m_holder.get();}
            implementation_type& operator * () const { return *m_holder;}

            void swap( basic_pimpl& other)
            {
               std::swap( m_holder, other.m_holder);
            }

         protected:

            //basic_pimpl( const basic_pimpl& other) : m_holder( new implementation_type( *other)) {}

            std::unique_ptr< implementation_type> m_holder;

         };
      } // move

      template< typename T>
      struct basic_pimpl : public move::basic_pimpl< T>
      {
         using base_type = move::basic_pimpl< T>;
         using implementation_type = T;

         basic_pimpl() = default;
         basic_pimpl( basic_pimpl&&) noexcept = default;
         basic_pimpl& operator = ( basic_pimpl&&) noexcept = default;


         //!
         //! Make a deep copy
         //! @note change to std::make_uniqeu
         //!
         basic_pimpl( const basic_pimpl& other)
         {
            m_holder( new implementation_type( *other));
         }

         //!
         //! Make a deep copy
         //!
         basic_pimpl& operator = ( const basic_pimpl& other)
         {
            basic_pimpl temporary{ other};
            std::swap( *this, other);
            return *this;
         }
      };

   } // common
} // casual

#endif // MOVE_H_
