//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/serialize/create.h"

#include "common/algorithm.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace create
         {
            namespace local
            {
               namespace
               {
                  template< typename Map, typename Out>
                  auto create( Map& map, const std::string& key, Out& out)
                  {
                     if( auto found = common::algorithm::find( map, key))
                        return found->second.create( out);
                     else 
                        throw exception::system::invalid::Argument{ common::string::compose( "failed to find archive creator for key: ", key)};
                  }

                  template< typename Map, typename Creator>
                  void registration( Map& map, Creator&& creator, const std::vector< std::string>& keys)
                  {
                     for( auto& key : keys)
                        map.emplace( key, creator);
                  }
               } // <unnamed>
            } // local

            namespace reader
            {
               namespace local
               {
                  namespace
                  {
                     enum class Policy
                     {
                        relaxed,
                        strict,
                        consumed
                     };
                     template< Policy policy> // just to get different instances based on policy
                     struct holder
                     {
                        static void registration( reader::Creator&& creator, const std::vector< std::string>& keys)
                        {
                           create::local::registration( creators(), std::move( creator), keys);
                        }

                        template< typename S>
                        static decltype( auto) from( const std::string& key, S& source)
                        {
                           return create::local::create( creators(), key, source);
                        }

                        static auto keys() 
                        {
                           return algorithm::transform( creators(), []( auto& pair){ return pair.first;});
                        }

                        static std::map< std::string, reader::Creator>& creators() 
                        {
                           static std::map< std::string, reader::Creator> creators;
                           return creators;
                        } 
                     };

                  } // <unnamed>
               } // local

               namespace consumed 
               {
                  using Dispatch = local::holder< local::Policy::consumed>;
                  namespace detail
                  {
                     void registration( reader::Creator&& creator, const std::vector< std::string>& keys)
                     {
                        Dispatch::registration( std::move( creator), keys);
                     }
                  } // detail
                  serialize::Reader from( const std::string& key, std::istream& stream) { return Dispatch::from( key, stream);}
                  serialize::Reader from( const std::string& key, platform::binary::type& data) { return Dispatch::from( key, data);}
               }

               namespace strict 
               {
                  using Dispatch = local::holder< local::Policy::strict>;
                  namespace detail
                  {
                     void registration( reader::Creator&& creator, const std::vector< std::string>& keys)
                     {
                        Dispatch::registration( std::move( creator), keys);
                     }
                  } // detail
                  serialize::Reader from( const std::string& key, std::istream& stream) { return Dispatch::from( key, stream);}
                  serialize::Reader from( const std::string& key, platform::binary::type& data) { return Dispatch::from( key, data);}
               }

               namespace relaxed 
               {
                  using Dispatch = local::holder< local::Policy::relaxed>;
                  namespace detail
                  {
                     void registration( reader::Creator&& creator, const std::vector< std::string>& keys)
                     {
                        Dispatch::registration( std::move( creator), keys);
                     }
                  } // detail
                  serialize::Reader from( const std::string& key, std::istream& stream) { return Dispatch::from( key, stream);}
                  serialize::Reader from( const std::string& key, platform::binary::type& data) { return Dispatch::from( key, data);}
               }

               std::vector< std::string> keys()
               {
                  return relaxed::Dispatch::keys();
               }

            } // reader
            namespace writer
            {
               namespace local
               {
                  namespace
                  {
                     namespace global
                     {
                        static std::map< std::string, writer::Creator>& creators() 
                        {
                           static std::map< std::string, writer::Creator> creators;
                           return creators;
                        } 
                     } // global
   
                     template< typename Out>
                     decltype( auto) create( const std::string& key, Out& out)
                     {
                         return serialize::create::local::create( global::creators(), key, out);
                     }
                  } // <unnamed>
               } // local

               namespace detail
               {
                  void registration( writer::Creator&& creator, const std::vector< std::string>& keys)
                  {
                     create::local::registration( local::global::creators(), std::move( creator), keys);
                  }
               } // detail


               serialize::Writer from( const std::string& key, std::ostream& stream)
               {
                  return local::create( key, stream);
               }

               serialize::Writer from( const std::string& key, platform::binary::type& data)
               {
                  return local::create( key, data);
               }

               std::vector< std::string> keys()
               {
                  return algorithm::transform( local::global::creators(), []( auto& pair){ return pair.first;});
               }

            } // writer
         } // create
      } // serialize
   } // common
} // casual