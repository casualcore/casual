//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/archive.h"
#include "common/serialize/policy.h"
#include "common/exception/system.h"

#include "casual/platform.h"

#include <string>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace create
         {
            namespace detail
            {
               template< typename M, typename... Ts>
               auto create( const std::string& key, M& map, Ts&&... ts)
               {
                  auto found = common::algorithm::find( map, key);

                  if( found)
                  {
                     return found->second->create( std::forward< Ts>( ts)...);
                  }
                  else 
                  {
                     throw exception::system::invalid::Argument{ common::string::compose( "failed to find archive creator for key: ", key)};
                  }
               }
            } // detail
           
            namespace writer
            {
               namespace detail
               {
                  // Some stuff to deduce what the archive implemantations are capable of.
                  //
                  // Hence, not force a specific semantics and give opportunities for better
                  // performance

                  namespace has
                  {
                     namespace detail
                     {
                        template< typename I, typename O>
                        using output_ctor = decltype( I( std::declval< O&>()));

                        template< typename I, typename O>
                        using output_flush = decltype( std::declval< I&>().flush( std::declval< O&>()));
                     } // detail

                     template< typename I, typename O>
                     using output_ctor = traits::detect::is_detected< detail::output_ctor, I, O>;

                     template< typename I, typename O>
                     using output_flush = traits::detect::is_detected< detail::output_flush, I, O>;
                     
                  } // has

                  namespace indirect
                  {
                     // using SFINAE-expressions and priority tag to prioritise the most sutable 
                     // flush implementation. It's a combinatorial problem and it seems priority tag 
                     // is the easiest way.

                     template< typename I>
                     auto flush( I& implementation, std::string& destination, traits::priority::tag< 0>) 
                        -> decltype( implementation.flush( std::declval< std::ostream&>()))
                     {
                        std::ostringstream stream;
                        implementation.flush( stream);
                        destination = std::move( stream).str();
                     }

                     template< typename I>
                     auto flush( I& implementation, std::string& destination, traits::priority::tag< 1>) 
                        -> decltype( implementation.flush( std::declval< platform::binary::type&>()))
                     {
                        platform::binary::type data;
                        implementation.flush( data);
                        common::algorithm::copy( data, destination);
                     }

                     template< typename I>
                     auto flush( I& implementation, platform::binary::type& destination, traits::priority::tag< 2>) 
                        -> decltype( implementation.flush( std::declval< std::ostream&>()))
                     {
                        std::ostringstream stream;
                        implementation.flush( stream);
                        common::algorithm::copy( stream.str(), destination);
                     }

                     // the most wanted indirection (highest tag#), implementation kan handle the destinaion type "natively"
                     // (hence, no indirection)
                     template< typename I, typename D>
                     auto flush( I& implementation, D& destination, traits::priority::tag< 3>) 
                        -> decltype( implementation.flush( destination))
                     {
                        implementation.flush( destination);
                     }
                     
                  } // indirect

                  template< typename D, typename I, typename Enable = void>
                  struct Flusher : I
                  {
                     Flusher( D& destination) : m_destination( destination) {}

                     void flush()
                     {
                        // make the indirect call to flush.
                        // we need to cast to implementation to get the correct `flush`
                        indirect::flush( static_cast< I&>( *this), m_destination.get(), traits::priority::tag< 3>{});
                     }
                     std::reference_wrapper< D> m_destination;
                  };

                  //! This is only for serialize::Log (at the moment). since Log only takes
                  //! std::ostream& in it's constructor we need to "indirect"
                  //! to binary::type. All output-archives need to handle at least binary::type
                  //! and std::ostream&. This is a consequence of the current design.
                  //! 
                  //! I find it very unlikely that we will ever use Log to a binary::type.
                  //! We might find a better solution in the future...
                  template< typename I>
                  struct Flusher< platform::binary::type, I, 
                        std::enable_if_t< 
                           ! has::output_flush< I, platform::binary::type>::value
                           && has::output_ctor< I, std::ostream>::value
                        >> 
                     : I
                  {
                     Flusher( platform::binary::type& destination) :I( m_stream), m_destination( destination) {}

                     void flush()
                     {
                        common::algorithm::copy( m_stream.str(), m_destination.get());
                     }
                     std::ostringstream m_stream;
                     std::reference_wrapper< platform::binary::type> m_destination;
                  };



                  namespace indirect
                  {
                     template< typename I, typename D, typename Enable = void>
                     struct creator;

                     //! uses the archive implementation constructor for the destination, if possible.
                     template< typename I, typename D>
                     struct creator< I, D, std::enable_if_t< has::output_ctor< I, D>::value>>
                     {
                        static auto create( D& destination)
                        {
                           return serialize::Writer::emplace< I>( destination);
                        }
                     };

                     //! uses the archive implementation::flush for the destination
                     template< typename I, typename D>
                     struct creator< I, D, std::enable_if_t< ! has::output_ctor< I, D>::value>>
                     {
                        static auto create( D& destination)
                        {
                           return serialize::Writer::emplace< Flusher< D, I>>( destination);
                        }
                     };
                     
                  } // indirect


                  template< typename I, typename D>
                  auto create( D& destination)
                  {
                     return indirect::creator< I, traits::remove_cvref_t< D>>::create( destination);
                  }

               } // detail


               struct Creator
               {
                  template< typename Implementation>
                  static Creator construct() 
                  {
                     return Creator{ Tag{}, []( auto& destination)
                     {
                        return detail::create< Implementation>( destination);
                     }};
                  } 

                  inline serialize::Writer create( std::ostream& stream) const  { return m_implementation->create( stream);}
                  inline serialize::Writer create( platform::binary::type& data) const { return m_implementation->create( data);}

               private:
                  
                  struct Tag{};

                  // using `Tag` to make sure we don't interfere with move/copy-ctor/assignment
                  template< typename C>
                  explicit Creator( Tag, C&& creator) 
                     : m_implementation{ std::make_shared< model< C>>( std::forward< C>( creator))} {}

                  struct concept
                  {
                     virtual ~concept() = default;
                     virtual serialize::Writer create( std::ostream& stream) const  = 0;
                     virtual serialize::Writer create( platform::binary::type& data) const = 0;
                  };

                  template< typename create_type> 
                  struct model : concept
                  {
                     model( create_type&& creator) : m_creator( std::move( creator)) {}

                     serialize::Writer create( std::ostream& stream) const override { return m_creator( stream);}
                     serialize::Writer create( platform::binary::type& data) const override  { return m_creator( data);}

                  private:
                     create_type m_creator;
                  };

                  std::shared_ptr< const concept> m_implementation;
               };

               static_assert( std::is_copy_constructible< Creator>::value, "");
               static_assert( std::is_copy_assignable< Creator>::value, "");

               


            } // writer
         } // create
      } // serialize
   } // common
} // casual
