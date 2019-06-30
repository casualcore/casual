//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/archive.h"
#include "common/serialize/policy.h"
#include "common/exception/system.h"

#include "common/platform.h"

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
            namespace reader
            {
              
               template< template< class> class P>
               struct basic_dispatch
               {
                  template< typename I>
                  using policy_type = P< I>;

                  static basic_dispatch& instance()
                  {
                     static basic_dispatch singleton;
                     return singleton;
                  }

                  template< typename I> 
                  bool registration( std::vector< std::string> keys)
                  {
                     auto creator = []( auto& source){
                        return serialize::Reader::emplace< policy_type< I>>( source);
                     };
                     auto model = make_shared( std::move( creator));
                     for( auto& key: keys)
                     {
                        m_creators.emplace( std::move( key), model);
                     }
                     return true;
                  }

                  serialize::Reader create( const std::string& key, std::istream& input) { return detail::create( key, m_creators, input);}
                  serialize::Reader create( const std::string& key, const platform::binary::type& input) { return detail::create( key, m_creators, input);}

               private:
                  basic_dispatch() = default;

                  template< typename C> 
                  auto make_shared( C&& creator)
                  {
                     return std::make_shared< Model< C>>( std::move( creator));
                  }

                  struct Base 
                  {
                     virtual ~Base() = default;
                     virtual serialize::Reader create( std::istream& stream) const  = 0;
                     virtual serialize::Reader create( const platform::binary::type& data) const = 0;
                  };

                  template< typename C> 
                  struct Model : Base
                  {
                     using create_type = C;

                     Model( create_type&& creator) : m_creator( std::move( creator)) {}

                     serialize::Reader create( std::istream& stream) const override 
                     { 
                        return m_creator( stream);
                     }
                     serialize::Reader create( const platform::binary::type& data) const override { return m_creator( data);}

                  private:
                     create_type m_creator;
                  };

                  using model_holder = std::map< std::string, std::shared_ptr< const Base>>;

                  model_holder m_creators;
               };

               namespace consumed 
               {
                  template< typename I, typename S> 
                  auto create( S&& source) { return Reader::emplace< policy::Consumed< I>>( std::forward< S>( source));}

                  using Dispatch = basic_dispatch< serialize::policy::Consumed>;

                  template< typename... Ts>
                  auto from( const std::string& key, Ts&&... ts)
                  {
                     return Dispatch::instance().create( key, std::forward< Ts>( ts)...);
                  }
               }

               namespace strict 
               {
                  template< typename I, typename S> 
                  auto create( S&& source) { return Reader::emplace< policy::Strict< I>>( std::forward< S>( source));}

                  using Dispatch = basic_dispatch< serialize::policy::Strict>;

                  template< typename... Ts>
                  auto from( const std::string& key, Ts&&... ts)
                  {
                     return Dispatch::instance().create( key, std::forward< Ts>( ts)...);
                  }
               }

               namespace relaxed 
               {
                  template< typename I, typename S> 
                  auto create( S&& source) { return Reader::emplace< policy::Relaxed< I>>( std::forward< S>( source));}

                  using Dispatch = basic_dispatch< serialize::policy::Relaxed>;

                  template< typename... Ts>
                  auto from( const std::string& key, Ts&&... ts)
                  {
                     return Dispatch::instance().create( key, std::forward< Ts>( ts)...);
                  }
               }

               template< typename I>
               struct Registration
               {
               private:
                  static bool m_dummy;
               };

               template< typename I> 
               CASUAL_OPTION_UNUSED bool Registration< I>::m_dummy = consumed::Dispatch::instance().registration< I>( I::keys())
                  && strict::Dispatch::instance().registration< I>( I::keys())
                  && relaxed::Dispatch::instance().registration< I>( I::keys());


            } // reader


            namespace writer
            {
               namespace detail
               {
                  // Some stuff to deduce what the archive implemantations are capable of.
                  //
                  // Hence, not force a specific semantics and give opportunities for better
                  // performance

                  template< typename I, typename O>
                  using has_output_ctor = decltype( I( std::declval< O&>()));

                  template< typename I, typename O>
                  using has_output_flush = decltype( std::declval< I&>().flush( std::declval< O&>()));


                  template< typename I, typename D>
                  auto do_flush( I& implementation, D& destination) -> std::enable_if_t< common::traits::detect::is_detected< has_output_flush, I, D>::value>
                  {
                     implementation.flush( destination);
                  }

                  template< typename I>
                  auto do_flush( I& implementation, std::string& destination) -> std::enable_if_t< 
                        ! common::traits::detect::is_detected< has_output_flush, I, std::string>::value
                        && common::traits::detect::is_detected< has_output_flush, I, std::ostream>::value
                     >
                  {
                     std::ostringstream stream;
                     implementation.flush( stream);
                     destination = stream.str();
                  }

                  template< typename I>
                  auto do_flush( I& implementation, std::string& destination) -> std::enable_if_t< 
                        ! common::traits::detect::is_detected< has_output_flush, I, std::string>::value
                        && ! common::traits::detect::is_detected< has_output_flush, I, std::ostream>::value
                        && common::traits::detect::is_detected< has_output_flush, I, platform::binary::type>::value
                     >
                  {
                     platform::binary::type data;
                     implementation.flush( data);
                     common::algorithm::copy( data, destination);
                  }

                  template< typename I>
                  auto do_flush( I& implementation, platform::binary::type& destination) -> std::enable_if_t< 
                        ! common::traits::detect::is_detected< has_output_flush, I, platform::binary::type>::value
                        && common::traits::detect::is_detected< has_output_flush, I, std::ostream>::value
                     >
                  {
                     std::ostringstream stream;
                     implementation.flush( stream);
                     common::algorithm::copy( stream.str(), destination);
                  }

                  template< typename D, typename I, typename Enable = void>
                  struct Flusher : I
                  {
                      Flusher( D& destination) : m_destination( destination) {}

                     void flush()
                     {
                        do_flush( static_cast< I&>( *this), m_destination.get());
                     }
                     std::reference_wrapper< D> m_destination;
                  };


                  template< typename I>
                  struct Flusher< platform::binary::type, I, 
                        std::enable_if_t< 
                           ! common::traits::detect::is_detected< has_output_flush, I, platform::binary::type>::value
                           && common::traits::detect::is_detected< has_output_ctor, I, std::ostream>::value
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

                  template< typename I, typename D, typename Enable = void>
                  struct basic_creator;

                  //! uses the archive implementation constructor for the destination, if possible.
                  template< typename I, typename D>
                  struct basic_creator< D, I, 
                     std::enable_if_t< common::traits::detect::is_detected< has_output_ctor, I, D>::value>>
                  {
                     static auto create( D& destination)
                     {
                        return serialize::Writer::emplace< I>( destination);
                     }
                  };

                  //! uses the archive implementation flush for the destination
                  template< typename I, typename D>
                  struct basic_creator< D, I, 
                     std::enable_if_t< ! common::traits::detect::is_detected< has_output_ctor, I, D>::value>>
                  {
                     static auto create( D& destination)
                     {
                        return serialize::Writer::emplace< Flusher< std::decay_t< D>, I>>( destination);
                     }
                  };


               } // detail

               template< typename I, typename D>
               serialize::Writer holder( D& destination)
               {
                  return detail::basic_creator< std::decay_t< D>, I>::create( destination);
               }

               struct Dispatch
               {
                  static Dispatch& instance();

                  template< typename I> 
                  bool registration( std::vector< std::string> keys)
                  {
                     auto creator = []( auto& destination){
                        return writer::holder< I>( destination);
                     };

                     auto model = make_shared( std::move( creator));

                     for( auto& key: keys)
                     {
                        m_creators.emplace( std::move( key), model);
                     }
                     return true;
                  }

                  serialize::Writer create( const std::string& key, std::ostream& stream);
                  serialize::Writer create( const std::string& key, platform::binary::type& data);

                  std::vector< std::string> keys() const;

               private:
                  Dispatch();

                  template< typename C> 
                  auto make_shared( C&& creator)
                  {
                     return std::make_shared< Model< C>>( std::move( creator));
                  }

                  struct Base 
                  {
                     virtual ~Base() = default;
                     virtual serialize::Writer create( std::ostream& stream) const  = 0;
                     virtual serialize::Writer create( platform::binary::type& data) const = 0;
                  };

                  template< typename C> 
                  struct Model : Base
                  {
                     using create_type = C;

                     Model( create_type&& creator) : m_creator( std::move( creator)) {}

                     serialize::Writer create( std::ostream& stream) const override { return m_creator( stream);}
                     serialize::Writer create( platform::binary::type& data) const override  { return m_creator( data);}

                  private:
                     create_type m_creator;
                  };
                  using model_holder = std::map< std::string, std::shared_ptr< const Base>>;

                  model_holder m_creators;
               };

               template< typename... Ts>
               auto from( const std::string& key, Ts&&... ts)
               {
                  return Dispatch::instance().create( key, std::forward< Ts>( ts)...);
               }

               template< typename P>
               struct Registration
               {
               private:
                  static bool m_dummy;
               };

               template< typename I> 
               CASUAL_OPTION_UNUSED bool Registration< I>::m_dummy = Dispatch::instance().registration< I>( I::keys());

               namespace complete
               {
                  inline auto format() 
                  {
                     return []( auto values, bool)
                     {
                        return Dispatch::instance().keys();
                     };
                  }
               } // complete

            } // writer
         } // create
      } // serialize
   } // common
} // casual
