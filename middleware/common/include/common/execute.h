//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/move.h"
#include "common/traits.h"
#include "common/exception/guard.h"
#include "common/serialize/macro.h"

#include <utility>
#include <functional>

namespace casual 
{
   namespace common::execute
   {
      //! executes an action ones.
      //! If the action has not been executed the
      //! destructor will perform the execution
      template< typename E>
      struct basic_scope
      {
         using execute_type = E;

         basic_scope( execute_type&& execute) : m_execute( std::move( execute)) {}

         ~basic_scope() noexcept
         {
            exception::guard( [&](){ (*this)();});
         }

         // Why does not this work?
         // error: exception specification of explicitly defaulted move assignment operator does not match the calculated one
         // 
         // move::Active is notrow, so it should work.
         //
         // using traits_type = traits::type< execute_type>;
         // basic_scope( basic_scope&&) noexcept( traits_type::nothrow_move_construct) = default;
         // basic_scope& operator = ( basic_scope&&) noexcept( traits_type::nothrow_move_assign) = default;

         basic_scope( basic_scope&&) = default;
         basic_scope& operator = ( basic_scope&&) = default;

         //! executes the actions ones.
         //! no-op if already executed
         void operator () ()
         {
            if( m_active)
            {
               m_execute();
               release();
            }
         }

         void release() { m_active.release();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_active, "active");
         )

      private:
         execute_type m_execute;
         move::Active m_active;
      };

      //! returns an executer that will do an action ones.
      //! If the action has not been executed the
      //! destructor will perform the execution
      template< typename E>
      auto scope( E&& executor)
      {
         return basic_scope< traits::remove_cvref_t< E>>{ std::forward< E>( executor)};
      }

   } // common::execute
} // casual 

