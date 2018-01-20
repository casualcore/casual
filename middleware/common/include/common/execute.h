//!
//! casual
//!

#ifndef CASUAL_COMMON_EXECUTE_H_
#define CASUAL_COMMON_EXECUTE_H_


#include "common/move.h"
#include "common/exception/handle.h"

#include <utility>

namespace casual 
{
   namespace common 
   {
      namespace execute 
      {
         //!
         //! executes an action ones.
         //! If the action has not been executed the
         //! destructor will perform the execution
         //!
         template< typename E>
         struct basic_scope
         {
            using execute_type = E;

            basic_scope( execute_type&& execute) : m_execute( std::move( execute)) {}

            ~basic_scope()
            {
               try
               {
                  (*this)();
               }
               catch( ...)
               {
                  exception::handle();
               }
            }

            basic_scope( basic_scope&&) noexcept = default;
            basic_scope& operator = ( basic_scope&&) noexcept = default;

            //!
            //! executes the actions ones.
            //! no-op if already executed
            //!
            void operator () ()
            {
               if( ! m_moved)
               {
                  m_execute();
                  release();
               }
            }

            void release() { m_moved.release();}

         private:
            execute_type m_execute;
            move::Moved m_moved;
         };

         //!
         //! returns an executer that will do an action ones.
         //! If the action has not been executed the
         //! destructor will perform the execution
         //!
         template< typename E>
         auto scope( E&& executor)
         {
            return basic_scope< E>{ std::forward< E>( executor)};
         }

         //!
         //! Execute @p executor once
         //!
         //! @note To be used with lambdas only.
         //! @note Every invocation of the same type will only execute once.
         //!
         template< typename E>
         void once( E&& executor)
         {
            static bool done = false;

            if( ! done)
            {
               done = true;
               executor();
            }
         }


      } // execute 
   } // common 
} // casual 


#endif