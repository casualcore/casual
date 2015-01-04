//!
//! casual_calling_context.h
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_


#include "common/queue.h"
#include "common/platform.h"
#include "common/message/server.h"



#include <vector>


namespace casual
{
   namespace common
   {


      namespace server
      {
         class Context;
      }

      namespace call
      {
         using descriptor_type = platform::descriptor_type;

         struct State
         {

            struct Pending
            {
               struct Descriptor
               {
                  Descriptor( descriptor_type descriptor, bool active = true)
                    : descriptor( descriptor), active( active) {}

                  descriptor_type descriptor;
                  bool active;

                  friend bool operator == ( descriptor_type cd, const Descriptor& d) { return cd == d.descriptor;}
                  friend bool operator == ( const Descriptor& d, descriptor_type cd) { return cd == d.descriptor;}
               };

               struct Transaction
               {
                  Transaction( const transaction::ID& trid, descriptor_type descriptor)
                    : m_trid( trid), m_involved{ descriptor} {}



                  bool remove( descriptor_type descriptor)
                  {
                     auto found = range::find( m_involved, descriptor);

                     if( found) { m_involved.erase( found.first);}

                     return m_involved.empty();
                  }

                  void add( descriptor_type descriptor) { m_involved.push_back( descriptor);}

                  friend bool operator == ( const Transaction& lhs, const transaction::ID& rhs) { return lhs.m_trid == rhs;}
                  friend bool operator == ( const Transaction& lhs, descriptor_type rhs) { return range::any_of( lhs.m_involved, std::bind( equal_to{}, std::placeholders::_1, rhs));}
               private:
                  transaction::ID m_trid;
                  std::vector< descriptor_type> m_involved;
               };

               struct Correlation
               {
                  Correlation( descriptor_type descriptor, const Uuid& correlation)
                   : descriptor( descriptor), correlation( correlation) {}

                  descriptor_type descriptor;
                  Uuid correlation;

                  friend bool operator == ( const Correlation& lhs, descriptor_type rhs) { return lhs.descriptor == rhs;}
               };

               Pending();

               //!
               //! Reserves a descriptor and associates it to message-correlation and transaction
               //!
               descriptor_type reserve( const Uuid& correlation, const transaction::ID& transaction);

               //!
               //! Reserves a descriptor and associates it to message-correlation
               //!
               descriptor_type reserve( const Uuid& correlation);

               void unreserve( descriptor_type descriptor);

               bool active( descriptor_type descriptor) const;

               const Uuid& correlation( descriptor_type descriptor) const;

            private:

               descriptor_type reserve();

               std::vector< Descriptor> m_descriptors;
               std::vector< Transaction> m_transactions;
               std::vector< Correlation> m_correlations;

            } pending;


            struct Reply
            {
               struct Cache
               {
                  typedef std::vector< message::service::Reply> cache_type;
                  using cache_range = decltype( range::make( cache_type::iterator(), cache_type::iterator()));


                  cache_range add( message::service::Reply&& value);

                  cache_range search( descriptor_type descriptor);

                  void erase( cache_range range);


               private:

                  cache_type m_cache;

               } cache;

            } reply;

            common::Uuid execution = common::uuid::make();

            std::string service;
         };

         class Context
         {
         public:
            static Context& instance();


            descriptor_type async( const std::string& service, char* idata, long ilen, long flags);

            void reply( descriptor_type& descriptor, char** odata, long& olen, long flags);

            void sync( const std::string& service, char* idata, const long ilen, char*& odata, long& olen, const long flags);

            int canccel( descriptor_type cd);

            void clean();

            void execution( const common::Uuid& uuid);
            const common::Uuid& execution() const;

            void service( const std::string& service);
            const std::string& service() const;

         private:

            using cache_range = State::Reply::Cache::cache_range;

            Context();

            bool receive( message::service::Reply& reply, descriptor_type descriptor, long flags);

            cache_range fetch( descriptor_type descriptor, long flags);

            void consume();

            State m_state;

         };
      } // call
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
