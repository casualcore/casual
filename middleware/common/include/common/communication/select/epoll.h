//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/platform.h"

#if defined(CASUAL_PLATFORM_LINUX)

#include "common/strong/id.h"
#include "common/functional.h"
#include "common/algorithm.h"
#include "common/communication/log.h"
#include "common/message/dispatch.h"


#include <vector>

namespace casual
{
   namespace common::communication::select
   {
      namespace directive
      {
 
         using range_type = range::type_t< std::vector< strong::file::descriptor::id>>;
         
         struct Set
         {
            void add( strong::file::descriptor::id descriptor) noexcept;
            void remove( strong::file::descriptor::id descriptor) noexcept;

            template< typename Range>
            auto add( Range&& descriptors) noexcept
               -> decltype( add( range::front( descriptors)))
            {
               algorithm::for_each( descriptors, [&]( auto descriptor){ add( descriptor);});
            }

            template< typename Range>
            auto remove( Range&& descriptors) noexcept
               -> decltype( remove( range::front( descriptors)))
            {
               algorithm::for_each( descriptors, [&]( auto descriptor){ remove( descriptor);});
            }

            inline auto descriptors() const noexcept
            { 
               return range::make( m_descriptors);
            }

            //! @returns the highest value descriptor in the `Set`, 'nil' descriptor if empty.
            inline auto highest() const noexcept { return m_highest;}
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_descriptors, "descriptors");
            )

         private:
            // mutable so we can 'filter' for ready - semantically we don't change the state
            mutable std::vector< strong::file::descriptor::id> m_descriptors;
            strong::file::descriptor::id m_highest{};
         };

         struct Ready
         {
            Ready( range_type read_ready, range_type write_ready)
               : m_descriptors( read_ready.size() + write_ready.size()), 
                  read{ range::make( std::begin( m_descriptors), read_ready.size())},
                  write{ range::make( std::end( read), write_ready.size())} 
            {
               algorithm::copy( read_ready, std::begin( read));
               algorithm::copy( write_ready, std::begin( write));
            }

         private:
            std::vector< strong::file::descriptor::id> m_descriptors;

         public:
            range_type read;
            range_type write;

            inline explicit operator bool() const noexcept { return read || write;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( read);
               CASUAL_SERIALIZE( write);
            )

         };

      } // directive


      struct Directive 
      {

         void read_add( strong::file::descriptor::id descriptor)
         { 
            m_read.add( descriptor); 
         }

         template< typename R>
         void read_remove( R&& descriptors)
         { 
            m_read.remove( std::forward< R>( descriptors)); 
         }

         void read_remove( strong::file::descriptor::id descriptor)
         { 
            m_read.remove( descriptor); 
         }

         void write_add( strong::file::descriptor::id descriptor)
         { 
            m_write.add( descriptor); 
         }

         template< typename R>
         void write_remove( R&& descriptors)
         {
            m_write.remove( std::forward< R>( descriptors));
         }

         void write_remove( strong::file::descriptor::id descriptor)
         { 
            m_write.remove( descriptor); 
         }

         //! removes `descriptor` from _read_ and _write_
         template< typename D>
         void remove( D&& descriptors) 
         { 
            m_read.remove( descriptors);
            m_write.remove( descriptors);
         }

         //! @returns the highest value descriptor in the `Directive`, 'nil' descriptor if empty.
         inline auto highest() const noexcept { return std::max( m_read.highest(), m_write.highest());}

         directive::Set m_read;
         directive::Set m_write;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_read);
            CASUAL_SERIALIZE( write);
         )
      };


      namespace dispatch::detail
      {
         directive::Ready select( const Directive& directive);

      } // dispatch::detail

   } // common::communication::select
} // casual

#endif // CASUAL_PLATFORM_LINUX
