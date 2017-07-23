//!
//! casual
//!

#include "common/transaction/resource.h"
#include "common/transaction/context.h"

#include "common/log/category.h"
#include "common/flag.h"

namespace casual
{

   namespace common
   {
      namespace transaction
      {

         namespace local
         {
            namespace
            {
               XID* non_const_xid( const Transaction& transaction)
               {
                  return const_cast< XID*>( &transaction.trid.xid);
               }

               Resource::code convert( int code)
               {
                  return static_cast<  Resource::code>( code);
               }

            } // <unnamed>
         } // local


         Resource::Resource( std::string key, xa_switch_t* xa, int id, std::string openinfo, std::string closeinfo)
            : key( std::move( key)), xa_switch( xa), id( id), openinfo( std::move( openinfo)), closeinfo( std::move( closeinfo))
         {
            log::category::transaction << "associated resource: " << *this << " name: '" <<  xa_switch->name << "' version: " << xa_switch->version << '\n';
         }

         Resource::Resource( std::string key, xa_switch_t* xa) : Resource( std::move( key), xa, 0, {}, {}) {}


         Resource::code Resource::start( const Transaction& transaction, long flags)
         {
            log::category::transaction << "start resource: " << *this << " transaction: " << transaction << " flags: " << std::hex << flags << std::dec << '\n';

            auto result = local::convert( xa_switch->xa_start_entry( local::non_const_xid( transaction), id, flags));


            if( result == code::duplicate_xid)
            {
               //
               // Transaction is already associated with this thread of control, we try to join instead
               //
               log::category::transaction << "XAER_DUPID - action: try to join instead\n";

               flags |= TMJOIN;

               result = local::convert( xa_switch->xa_start_entry( local::non_const_xid( transaction), id, flags));
            }

            if( result != code::ok)
            {
               log::category::error << std::error_code( result) << " failed to start resource - " << *this << " - trid: " << transaction << '\n';
            }
            return result;
         }

         Resource::code Resource::end( const Transaction& transaction, long flags)
         {
            log::category::transaction << "end resource: " << *this << " transaction: " << transaction << " flags: " << std::hex << flags << std::dec << '\n';

            auto result = local::convert( xa_switch->xa_end_entry( local::non_const_xid( transaction), id, flags));

            if( result != code::ok)
            {
               log::category::error << std::error_code( result) << " failed to end resource - " << *this << " - trid: " << transaction << '\n';
            }
            return result;

         }

         Resource::code Resource::open( long flags)
         {
            log::category::transaction << "open resource: " << *this <<  " flags: " << std::hex << flags << std::dec << '\n';

            auto result = local::convert( xa_switch->xa_open_entry( openinfo.c_str(), id, flags));

            if( result != code::ok)
            {
               log::category::error << std::error_code( result) << " failed to open resource - " << *this << '\n';
            }

            return result;
         }

         Resource::code Resource::close( long flags)
         {
            log::category::transaction << "close resource: " << *this <<  " flags: " << std::hex << flags << std::dec <<'\n';

            auto result = local::convert( xa_switch->xa_close_entry( closeinfo.c_str(), id, flags));

            if( result != code::ok)
            {
               log::category::error << std::error_code( result) << " failed to close resource - " << *this << '\n';
            }

            return result;
         }

         Resource::code Resource::commit( const Transaction& transaction, long flags)
         {
            log::category::transaction << "commit resource: " << *this <<  " flags: " << std::hex << flags << std::dec <<'\n';

            return local::convert( xa_switch->xa_commit_entry( local::non_const_xid( transaction), id, flags));
         }

         Resource::code Resource::rollback( const Transaction& transaction, long flags)
         {
            log::category::transaction << "rollback resource: " << *this <<  " flags: " << std::hex << flags << std::dec <<'\n';

            return local::convert( xa_switch->xa_rollback_entry( local::non_const_xid( transaction), id, flags));
         }

         bool Resource::dynamic() const
         {
            return common::flag< TMREGISTER>( xa_switch->flags);
         }


         std::ostream& operator << ( std::ostream& out, const Resource& resource)
         {
            return out << "{ key: " << resource.key 
               << ", id: " << resource.id 
               << ", openinfo: " << resource.openinfo
               << ", closeinfo: " << resource.closeinfo 
               << "}";
         }


      } //transaction
   } // common

} // casual
