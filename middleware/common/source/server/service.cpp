//!
//!
//! casual
//!

#include "common/server/service.h"
#include "common/server/context.h"
#include "common/log.h"

#include "common/algorithm.h"
#include "common/memory.h"
#include "common/buffer/pool.h"

#include "common/exception/handle.h"



namespace casual
{
   namespace common
   {
      namespace server
      {

         Service::Service( std::string name, function_type function, service::transaction::Type transaction, std::string category)
           : name( std::move( name)), function( std::move( function)), transaction( transaction), category( std::move( category)) {}

         Service::Service( std::string name, function_type function)
         : name( std::move( name)), function( std::move( function)) {}

         service::invoke::Result Service::operator () ( service::invoke::Parameter&& argument)
         {
            return function( std::move( argument));
         }

         std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{ name: " << service.name
                  << ", category: " << service.category
                  << ", transaction: " << service.transaction
                  << ", compare: " << service.compare
                  << '}';
         }

         bool operator == ( const Service& lhs, const Service& rhs)
         {
            return lhs == rhs.compare;
         }

         bool operator == ( const Service& lhs, const void* rhs)
         {
            if( lhs.compare && rhs)
            {
               return lhs.compare == rhs;
            }
            return false;
         }
         bool operator != ( const Service& lhs, const Service& rhs)
         {
            return ! ( lhs == rhs);
         }


         namespace xatmi
         {
            namespace local
            {
               namespace
               {

                  namespace transform
                  {
                     TPSVCINFO information( service::invoke::Parameter& argument)
                     {
                        Trace trace{ "server::xatmi::local::transform::information"};

                        TPSVCINFO result;

                        //
                        // Before we call the user function we have to add the buffer to the "buffer-pool"
                        //
                        memory::copy( range::make( argument.service.name), range::make( result.name));

                        result.len = argument.payload.memory.size();
                        result.cd = argument.descriptor;
                        result.flags = argument.flags.underlaying();

                        //
                        // This is the only place where we use adopt
                        //
                        result.data = buffer::pool::Holder::instance().adopt( std::move( argument.payload));

                        return result;
                     }

                     buffer::Payload payload( const server::state::Jump& jump)
                     {
                        if( jump.buffer.data != nullptr)
                        {
                           return buffer::pool::holder().release( jump.buffer.data, jump.buffer.size);
                        }

                        return { nullptr};
                     }

                     service::invoke::Result result( const server::state::Jump& jump)
                     {
                        service::invoke::Result result{ transform::payload( jump)};

                        result.code = jump.state.code;
                        result.transaction = jump.state.value == flag::xatmi::Return::success ?
                              service::invoke::Result::Transaction::commit : service::invoke::Result::Transaction::rollback;

                        return result;
                     }

                     service::invoke::Forward forward( const server::state::Jump& jump)
                     {
                        service::invoke::Forward result;

                        result.parameter.payload =  transform::payload( jump);
                        result.parameter.service.name = jump.forward.service;

                        return result;
                     }

                  } // transform

                  struct Invoke
                  {
                     Invoke( function_type function) : m_function( std::move( function))
                     {

                     }

                     service::invoke::Result operator () ( service::invoke::Parameter&& argument)
                     {
                        auto& state = server::context().state();

                        //
                        // Set destination for the coming jump...
                        // we can't wrap the jump in some abstraction since it's
                        // UB (http://en.cppreference.com/w/cpp/utility/program/setjmp)
                        //
                        switch( ::setjmp( state.jump.environment))
                        {
                           case state::Jump::Location::c_no_jump:
                           {
                              invoke( argument);

                              //
                              // User service returned, not by tpreturn.
                              //
                              throw exception::xatmi::service::Error( "service: " + argument.service.name + " did not call tpreturn");
                           }
                           case state::Jump::Location::c_forward:
                           {
                              log::debug << "user called tpforward\n";

                              throw transform::forward( state.jump);
                           }
                           default:
                           {
                              throw common::exception::system::invalid::Argument{ "unexpected value from setjmp"};
                           }
                           case state::Jump::Location::c_return:
                           {
                              log::debug << "user called tpreturn\n";

                              return transform::result( state.jump);
                           }
                        }
                     }

                  private:

                     void invoke( service::invoke::Parameter& argument)
                     {
                        //
                        // Also takes care of buffer to pool
                        //
                        TPSVCINFO information = transform::information( argument);

                        try
                        {
                           m_function( &information);
                        }
                        catch( ...)
                        {
                           exception::handle();
                           log::category::error << "exception thrown from service: " << argument.service.name << std::endl;
                        }

                     }

                     function_type m_function;
                  };



               } // <unnamed>
            } // local

            server::Service service( std::string name, function_type function, service::transaction::Type transaction, std::string category)
            {
               auto compare = address( function);
               server::Service result{ std::move( name), local::Invoke{ std::move( function)}, transaction, std::move( category)};
               result.compare = compare;

               return result;
            }

            server::Service service( std::string name, function_type function)
            {
               return service( std::move( name), std::move( function), service::transaction::Type::automatic, {});
            }

            const void* address( const function_type& function)
            {
               auto target = function.target<void(*)(TPSVCINFO*)>();

               if( target) { return reinterpret_cast< void*>( *target);}

               return target;
            }

         } // xatmi



      } // server
   } // common
} // casual
