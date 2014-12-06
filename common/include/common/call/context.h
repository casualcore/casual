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

#include <set>
#include <unordered_map>
#include <unordered_set>

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

         struct State
         {
            using descriptor_type = platform::descriptor_type;

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

               int reserve();

               void unreserve( descriptor_type descriptor);

               bool active( descriptor_type descriptor) const;

            private:
               typedef std::vector< Descriptor> descriptors_type;
               descriptors_type m_descriptors;

            } pending;


            struct Reply
            {
               struct Cache
               {
                  typedef std::deque< message::service::Reply> cache_type;
                  using cache_range = decltype( range::make( cache_type::iterator(), cache_type::iterator()));


                  cache_range add( message::service::Reply&& value);

                  cache_range search( descriptor_type descriptor);

                  void erase( cache_range range);


               private:

                  cache_type m_cache;

               } cache;
            } reply;


            typedef std::deque< message::service::Reply> reply_cache_type;
            using cache_range = decltype( range::make( reply_cache_type().begin(), reply_cache_type().end()));


            common::Uuid execution = common::uuid::make();

            std::string currentService;
         };

         class Context
         {
         public:
            static Context& instance();


            int asyncCall( const std::string& service, char* idata, long ilen, long flags);

            void getReply( int* idPtr, char** odata, long& olen, long flags);

            int canccel( int cd);

            void clean();

            void execution( const common::Uuid& uuid);
            const common::Uuid& execution() const;

            void currentService( const std::string& service);
            const std::string& currentService() const;

         private:




            using cache_range = State::cache_range;

            Context();


            cache_range fetch( int descriptor, long flags);

            void consume();

            State m_state;

         };
      } // call
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
