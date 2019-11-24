//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/serialize/json.h"
#include "common/serialize/create.h"
#include "common/exception/casual.h"

#include "common/transcode.h"
#include "common/functional.h"
#include "common/buffer/type.h"

// TODO: Move this to makefile
#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <iterator>
#include <istream>
#include <functional>

#include <iostream>


namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace json
         {
            namespace local
            {
               namespace
               {
                  std::vector< std::string> keys() { return { "json", "jsn", common::buffer::type::json()};};

                  namespace reader
                  {
                     namespace check
                     {
                        // This is a help to check some to avoid terminate (via assert)
                        template<typename C, typename F>
                        auto read( const rapidjson::Value* const value, C&& checker, F&& fetcher)
                        {
                           if( common::invoke( checker, value))
                           {
                              return common::invoke( fetcher, value);
                           }

                           throw exception::casual::invalid::Node{ "unexpected type"};
                        }
                     } // check

                            
                     const rapidjson::Document& parse( rapidjson::Document& document, const char* const json)
                     {
                        // To support empty documents
                        if( ! json || json[ 0] == '\0')
                        {
                           document.Parse( "{}");
                        }
                        else
                        {
                           document.Parse( json);
                        }

                        if( document.HasParseError())
                        {
                           throw exception::casual::invalid::Document{ rapidjson::GetParseError_En( document.GetParseError())};
                        }
                        return document;
                     }



                     const rapidjson::Document& parse( rapidjson::Document& document, std::istream& stream)
                     {
                        // note: istreambuf_iterator does not skip whitespace, which is what we want.
                        const std::string buffer{
                           std::istreambuf_iterator<char>(stream),
                           {}};

                        return parse( document, buffer.c_str());
                     }

                     const rapidjson::Document& parse( rapidjson::Document& document, const std::string& json)
                     {
                        return parse( document, json.c_str());
                     }

                     const rapidjson::Document& parse( rapidjson::Document& document, const char* const json, const platform::size::type size)
                     {
                        // To ensure null-terminated string
                        const std::string buffer{ json, json + size};
                        return parse( document, buffer.c_str());
                     }

                     const rapidjson::Document& parse( rapidjson::Document& document, const platform::binary::type& json)
                     {
                        if( ! json.empty() && json.back() == '\0')
                           return parse( document, json.data());
                        else
                           return parse( document, json.data(), json.size());
                     }

                     namespace canonical
                     {
                        struct Parser 
                        {
                           using Node = rapidjson::Value;
                           auto operator() ( const Node& document)
                           {
                              deduce( document, nullptr);
                              return std::exchange( m_canonical, {});
                           }

                        private:

                           void deduce( const Node& node, const char* name)
                           {
                              switch( node.GetType())
                              { 
                                 case rapidjson::Type::kNumberType: scalar( node, name); break;
                                 case rapidjson::Type::kStringType: scalar( node, name); break;
                                 case rapidjson::Type::kTrueType: scalar( node, name); break;
                                 case rapidjson::Type::kFalseType: scalar( node, name); break;
                                 case rapidjson::Type::kArrayType: sequence( node, name); break;
                                 case rapidjson::Type::kObjectType: map( node, name); break;
                                 case rapidjson::Type::kNullType: /*???*/ break;
                              }
                           }

                           void scalar( const Node& node, const char* name)
                           {
                              m_canonical.attribute( name);
                           }

                           void sequence( const Node& node, const char* name)
                           {
                              start( name);

                              for( auto current = node.Begin(); current != node.End(); ++current)
                              {
                                 deduce( *current, "element");  
                              }
                              
                              end( name);
                           }

                           void map( const Node& node, const char* name)
                           {
                              start( name);

                              for( auto current = node.MemberBegin(); current != node.MemberEnd(); ++current)
                              {
                                 deduce( current->value, current->name.GetString());
                              }
                              
                              end( name);
                           }

                           void start( const char* name)
                           {
                              // take care of the first node which doesn't have a name, and is
                              // not a composite in an archive sense.
                              if( name) 
                                 m_canonical.composite_start( name);
                           }

                           void end( const char* name)
                           {
                              // take care of the first node which doesn't have a name, and is
                              // not a composite in an archive sense.
                              if( name)
                                 m_canonical.composite_end();
                           }
                           policy::canonical::Representation m_canonical;
                        };

                        auto parse( const rapidjson::Value& document)
                        {
                           return Parser{}( document);
                        }

                     } // canonical

                     class Implementation
                     {
                     public:

                        inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

                        static decltype( auto) keys() { return local::keys();}

                        template< typename... Ts>
                        explicit Implementation( Ts&&... ts) : m_stack{ & reader::parse( m_document, std::forward< Ts>( ts)...)} 
                        {
                           
                        }
                        ~Implementation() = default;

                        std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* const name)
                        {
                           if( ! start( name))
                           {
                              return std::make_tuple( 0, false);
                           }

                           const auto& node = *m_stack.back();

                           // This check is to avoid terminate (via assert)
                           if( ! node.IsArray())
                           {
                              throw exception::casual::invalid::Node{ "expected array"};
                           }

                           // Stack 'em backwards

                           size = node.Size();

                           for( auto index = size; index > 0; --index)
                           {
                              m_stack.push_back( &node[ index - 1]);
                           }

                           return std::make_tuple( size, true);

                        }

                        void container_end( const char* const name)
                        {
                           end( name);
                        }

                        bool composite_start( const char* const name)
                        {
                           if( ! start( name))
                           {
                              return false;
                           }

                           // This check is to avoid terminate (via assert)
                           if( ! m_stack.back()->IsObject())
                           {
                              throw exception::casual::invalid::Node{ "expected object"};
                           }

                           return true;

                        }

                        void composite_end(  const char* const name)
                        {
                           end( name);
                        }

                        bool start( const char* const name)
                        {
                           if( name)
                           {
                              const auto result = m_stack.back()->FindMember( name);

                              if( result != m_stack.back()->MemberEnd())
                              {
                                 m_stack.push_back( &result->value);
                              }
                              else
                              {
                                 return false;
                              }
                           }

                           // Either we found the node or we assume it's an 'unnamed' container
                           // element that is already pushed to the stack

                           return true;
                        }

                        void end( const char* const name)
                        {
                           m_stack.pop_back();
                        }

                        template< typename T>
                        bool read( T& value, const char* name)
                        {
                           if( start( name))
                           {
                              if( m_stack.back()->IsNull())
                              {
                                 // Act (somehow) relaxed

                                 value = T{};
                              }
                              else
                              {
                                 read( value);
                              }
                              end( name);

                              return true;
                           }

                           return false;
                        }

                        void read( bool& value) const
                        { value = check::read( m_stack.back(), &rapidjson::Value::IsBool, &rapidjson::Value::GetBool); }
                        void read( short& value) const
                        { value = check::read( m_stack.back(), &rapidjson::Value::IsInt, &rapidjson::Value::GetInt); }
                        void read( long& value) const
                        { value = check::read( m_stack.back(), &rapidjson::Value::IsInt64, &rapidjson::Value::GetInt64); }
                        void read( long long& value) const
                        { value = check::read( m_stack.back(), &rapidjson::Value::IsInt64, &rapidjson::Value::GetInt64); }
                        void read( float& value) const
                        { value = check::read( m_stack.back(), &rapidjson::Value::IsNumber, &rapidjson::Value::GetDouble); }
                        void read( double& value) const
                        { value = check::read( m_stack.back(), &rapidjson::Value::IsNumber, &rapidjson::Value::GetDouble); }
                        void read( char& value) const
                        { value = *common::transcode::utf8::decode( check::read( m_stack.back(), &rapidjson::Value::IsString, &rapidjson::Value::GetString)).c_str(); }
                        void read( std::string& value) const
                        { value = common::transcode::utf8::decode( check::read( m_stack.back(), &rapidjson::Value::IsString, &rapidjson::Value::GetString)); }
                        void read( platform::binary::type& value) const
                        { value = common::transcode::base64::decode( check::read( m_stack.back(), &rapidjson::Value::IsString, &rapidjson::Value::GetString)); }

                        void read( view::Binary value) const
                        { 
                           auto binary = common::transcode::base64::decode( 
                              check::read( m_stack.back(), &rapidjson::Value::IsString, &rapidjson::Value::GetString));
                           
                           if( range::size( binary) != range::size( value))
                              throw exception::casual::invalid::Node{ "binary size missmatch"};

                           algorithm::copy( binary, std::begin( value));
                        }
                     
                        policy::canonical::Representation canonical()
                        {
                           return canonical::parse( m_document);
                        }
                     
                     private:

                        rapidjson::Document m_document;
                        std::vector<const rapidjson::Value*> m_stack;
                     };

                  } // reader
                  
                  
                  namespace writer
                  {
                     class Implementation
                     {
                     public:

                        inline constexpr static auto archive_type() { return archive::Type::static_need_named;}

                        static decltype( auto) keys() { return local::keys();}
                        
                        explicit Implementation()
                           : m_allocator( m_document.GetAllocator()), m_stack{ &m_document}
                        {
                           m_document.SetObject();
                        }
                        
                        //Implementation( rapidjson::Value& object, rapidjson::Document::AllocatorType& allocator);
                        ~Implementation() = default;

                        platform::size::type container_start( platform::size::type size, const char* const name)
                        {
                           start( name);
                           m_stack.back()->SetArray();

                           return size;
                        }

                        void container_end( const char* const name)
                        {
                           end( name);
                        }

                        void composite_start( const char* const name)
                        {
                           start( name);
                           m_stack.back()->SetObject();
                        }
                        void composite_end(  const char* const name)
                        {
                           end( name);
                        }

                        void start( const char* const name)
                        {
                           // Both AddMember and PushBack returns the parent (*this)
                           // instead of a reference to the added value (despite what
                           // the documentation says) and thus we need to do some
                           // cumbersome iterator stuff to get a reference/pointer to it

                           auto& parent = *m_stack.back();

                           if( name)
                           {
                              parent.AddMember( rapidjson::Value( name, m_allocator), rapidjson::Value(), m_allocator);

                              m_stack.push_back( &(*(parent.MemberEnd() - 1)).value);
                           }
                           else
                           {
                              // We are in a container
                              parent.PushBack( rapidjson::Value(), m_allocator);
                              m_stack.push_back( &(*(parent.End() - 1)));
                           }
                        }

                        template< typename T>
                        void write( const T& value, const char* name)
                        {
                           start( name);
                           write( value);
                           end( name);
                        }
                        
                        void end( const char* name)
                        {
                           m_stack.pop_back();
                        }


                        void write( const bool value) { m_stack.back()->SetBool( value); }
                        void write( const char value) { write( std::string{ value}); }
                        void write( const short value) { m_stack.back()->SetInt( value); }
                        void write( const long value) { m_stack.back()->SetInt64( value); }
                        void write( const long long value) { m_stack.back()->SetInt64( value); }
                        void write( const float value) { m_stack.back()->SetDouble( value); }
                        void write( const double value) { m_stack.back()->SetDouble( value); }
                        void write( const std::string& value) { m_stack.back()->SetString( common::transcode::utf8::encode( value), m_allocator);}
                        void write( const platform::binary::type& value) { m_stack.back()->SetString( common::transcode::base64::encode( value), m_allocator);}
                        void write( view::immutable::Binary value) { m_stack.back()->SetString( common::transcode::base64::encode( value), m_allocator);}

                        const rapidjson::Document& document() const { return m_document;}

                        void flush( platform::binary::type& json)
                        {
                           rapidjson::StringBuffer buffer;
                           rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( buffer);

                           if( m_document.Accept( writer))
                           {
                              json.assign( buffer.GetString(), buffer.GetString() + buffer.GetSize());
                           }
                           else
                           {
                              // TODO: Better
                              throw exception::casual::invalid::Document{ "Failed to write document"};
                           }
                        }

                        void flush( std::ostream& json)
                        {
                           rapidjson::StringBuffer buffer;
                           rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( buffer);
                           if( m_document.Accept( writer))
                           {
                              json << buffer.GetString();
                           }
                           else
                           {
                              // TODO: Better
                              throw exception::casual::invalid::Document{ "Failed to write document"};
                           }
                        }

                     private:

                        rapidjson::Document m_document;
                        rapidjson::Document::AllocatorType& m_allocator;
                        std::vector< rapidjson::Value*> m_stack;
                     };

                  } // writer
               } // <unnamed>
            } // local

            namespace strict
            {
               serialize::Reader reader( const std::string& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::strict::create< local::reader::Implementation>( source);}
            } // strict

            namespace relaxed
            {    
               serialize::Reader reader( const std::string& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::relaxed::create< local::reader::Implementation>( source);}
            } // relaxed

            namespace consumed
            {    
               serialize::Reader reader( const std::string& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( std::istream& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
               serialize::Reader reader( const platform::binary::type& source) { return create::reader::consumed::create< local::reader::Implementation>( source);}
            } // consumed

            serialize::Writer writer( std::string& destination)
            {
               return serialize::create::writer::create< local::writer::Implementation>( destination);
            }

            serialize::Writer writer( std::ostream& destination)
            {
               return serialize::create::writer::create< local::writer::Implementation>( destination);
            }

            serialize::Writer writer( platform::binary::type& destination)
            {
               return serialize::create::writer::create< local::writer::Implementation>( destination);
            }

         } // json
         
         namespace create
         {
            namespace reader
            {
               template struct Registration< json::local::reader::Implementation>;
            } // writer
            namespace writer
            {
               template struct Registration< json::local::writer::Implementation>;
            } // writer
         } // create

      } // serialize
   } // common
} // casual
