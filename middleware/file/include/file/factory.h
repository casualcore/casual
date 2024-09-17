#include "file/message.h"

#include "common/process.h"
#include "common/transaction/context.h"

namespace casual
{
   namespace file::factory
   {

      namespace create
      {
         namespace blocking
         {
            inline auto request( std::filesystem::path path)
            {
               auto& transaction = common::transaction::context().current();

               if( transaction)
               {
                  // make sure to trigger an interaction with the TM
                  transaction.external();
               }
               else
               {
                  common::code::raise::error( common::code::casual::preconditions, "no transaction");
               }

               message::reserve::Request result;

               result.process = common::process::handle();

               result.trid = transaction.trid;
               result.path = std::move( path);

               return result;
            }
         } // blocking


         namespace non
         {
            namespace blocking
            {
               inline auto request( std::filesystem::path path)
               {
                  auto result = create::blocking::request( std::move( path));
                  result.wait = false;
                  return result;
               }
            } // blocking
         } // non

      } // create

   } // file::factory
} // casual