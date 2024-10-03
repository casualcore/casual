#include "file/resource/logic.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <format>

namespace casual::file::resource
{
   namespace local
   {
      namespace
      {
         auto encode( const auto& transaction)
         {
            return (std::ostringstream{} << transaction).str();
         }

         auto temporary( const common::transaction::ID& transaction, const std::filesystem::path& path)
         {
            std::filesystem::path result{ path};
            result += ".casual.";
            result += encode( transaction.xid);
            return result;
         }

         std::unordered_map< common::transaction::ID, std::unordered_set< std::filesystem::path>> transactions;
      } //
   } // local

   std::filesystem::path acquire( const common::transaction::ID& transaction, std::filesystem::path origin)
   {
      auto result = local::temporary( transaction, origin);

      if (std::filesystem::exists( origin))
      {
         std::filesystem::copy_file( origin, result);
      }

      local::transactions[ transaction].insert( std::move( origin));

      return result;
   }

   void perform( const common::transaction::ID& transaction)
   {
      if( local::transactions.contains( transaction))
      {
         for( const auto &origin : local::transactions[ transaction])
         {
            const auto rushes = local::temporary( transaction, origin);

            if (std::filesystem::exists( rushes))
            {
               std::filesystem::rename( rushes, origin);
            }
            else
            {
               std::filesystem::remove_all( origin);
            }
         }

         local::transactions.erase( transaction);
      }
   }

   void restore( const common::transaction::ID& transaction)
   {
      if( local::transactions.contains( transaction))
      {
         for( const auto &origin : local::transactions[ transaction])
         {
            const auto rushes = local::temporary( transaction, origin);

            if( std::filesystem::exists( rushes))
            {
               std::filesystem::remove_all( rushes);
            }
         }

         local::transactions.erase( transaction);
      }
   }

} // casual::file::resource
