//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/buffer/field.h"
#include "casual/platform.h"

#include <functional>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <cstring>
#include <type_traits>
#include <sstream>

namespace casual
{
   namespace buffer
   {
      namespace internal
      {
         namespace field
         {
            namespace detail
            {
               /*
               #define CASUAL_FIELD_SHORT 1
               #define CASUAL_FIELD_LONG 2
               #define CASUAL_FIELD_CHAR 3
               #define CASUAL_FIELD_FLOAT 4  // not implemented yet
               #define CASUAL_FIELD_DOUBLE 5 // not implemented yet
               #define CASUAL_FIELD_STRING 6
               #define CASUAL_FIELD_BINARY 7 // not implemented yet
               */

               template< decltype( CASUAL_FIELD_STRING)>
               struct base_field_traits;

               template<>
               struct base_field_traits< CASUAL_FIELD_STRING>
               {
                  using value_type = const char*;
                  using format_value_type = std::string;
                  static constexpr auto get = &casual_field_get_string;

                  inline static auto add( char** buffer, long id, value_type value) { return casual_field_add_string( buffer, id, value);}
                  inline static auto add( char** buffer, long id, const format_value_type& value) { return casual_field_add_string( buffer, id, value.c_str());}

                  template< typename S>
                  static auto consume( S& stream, long count) { return stream.consume( count);}
               };

               struct base_consume_traits
               {
                  template< typename S>
                  static auto consume( S& stream, long count) 
                  {
                     auto result = stream.consume( count);

                     if( result.size != count)
                        throw std::out_of_range{ "stream buffer is depleted"};

                     return result;
                  }
               };

               template<>
               struct base_field_traits< CASUAL_FIELD_SHORT> : base_consume_traits
               {
                  using value_type = short;
                  using format_value_type = value_type;
                  static constexpr auto get = &casual_field_get_short;
                  static constexpr auto add = &casual_field_add_short;
               };

               template<>
               struct base_field_traits< CASUAL_FIELD_CHAR> : base_consume_traits
               {
                  using value_type = char;
                  using format_value_type = value_type;
                  static constexpr auto get = &casual_field_get_char;
                  static constexpr auto add = &casual_field_add_char;
               };               

               template<>
               struct base_field_traits< CASUAL_FIELD_LONG> : base_consume_traits
               {
                  using value_type = long;
                  using format_value_type = value_type;
                  static constexpr auto get = &casual_field_get_long;
                  static constexpr auto add = &casual_field_add_long;
               };

               template< long id>
               struct field_traits : base_field_traits< id / CASUAL_FIELD_TYPE_BASE> {};


               //! checks field result and throws if errors...
               void check( int result);
               
            } // detail
            namespace stream 
            {
               
               struct Input
               {
                  Input( const char* buffer) : m_buffer{ buffer} {};

                  template< long id> 
                  auto get()
                  {
                     using traits = detail::field_traits< id>;
                     typename traits::value_type value;
                     detail::check( traits::get( m_buffer, id, m_index[ id]++, &value));
                     return value;
                  };

               private:
                  const char* m_buffer;
                  std::map< long, long> m_index;
               };

               struct Output
               {
                  Output( char** buffer) : m_buffer{ buffer} {};

                  template< long id, typename V>
                  auto add( V&& value)
                  {
                     using traits = detail::field_traits< id>;
                     detail::check( traits::add( m_buffer, id, std::forward< V>( value)));
                  };

               private:
                  char** m_buffer;
               };

            } // stream

            namespace string
            {
               template< typename Data>
               struct view
               {
                  view( Data data, long size) : data{ data}, size{ size} {}
                  view() = default;

                  Data data{};
                  long size = 0;

                  auto begin() { return data;}
                  auto begin() const { return data;}
                  auto end() { return data + size;}
                  auto end() const { return data + size;}
               };

               namespace stream
               {
                  template< typename View>
                  struct basic_stream
                  {
                     using view_type = View;

                     basic_stream( view_type view) 
                        : m_view{ view} {}

                     view_type consume( long count)
                     {
                        count = std::min( count, capacity() - size());

                        view_type result{ m_view.data + m_offset, count};
                        m_offset += count;

                        return result;
                     };

                     long size() const { return m_offset;}
                     long capacity() const { return m_view.size;}

                     view_type view() const { return { m_view.data, m_offset};}

                  private:
                     view_type m_view;
                     long m_offset = 0;
                  };

                  using Input = basic_stream< string::view< const char*>>;
                  using Output = basic_stream< string::view< char*>>;

               } // stream
               
               enum class Alignment : short
               {
                  left,
                  right
               };

               namespace detail
               {
                  template< string::Alignment alignment>
                  struct format;

                  template<>
                  struct format< string::Alignment::left>
                  {
                     // to string
                     void operator() ( const char* value, stream::Output::view_type destination, char padding)
                     {
                        auto current = std::begin( destination);
                        while( *value != '\0' && current != std::end( destination))
                           *current++ = *value++;

                        // pad the rest, if any
                        std::fill( current, std::end( destination), padding);
                     }
                     
                     // to string
                     template< std::integral T>
                     auto operator() ( T value, stream::Output::view_type destination, char padding)
                     {
                        auto string = std::to_string( value);
                        operator()( string.c_str(), destination, padding);
                     }

                     // from string
                     void operator() ( stream::Input::view_type source, std::string& value, char padding)
                     {
                        // find the first 'non-padding' from the back.
                        auto last = std::find_if( 
                           std::make_reverse_iterator( std::end( source)), 
                           std::make_reverse_iterator( std::begin( source)),
                           [padding]( auto c){ return c != padding;}).base();

                        value.resize( std::distance( std::begin( source), last));
                        std::copy( std::begin( source), last, std::begin( value));
                     }

                     // from string
                     template< std::integral T>
                     auto operator() ( stream::Input::view_type source, T& value, char padding)
                     {
                        std::string string;
                        operator()( source, string, padding);
                        std::istringstream converter{ std::move( string)};
                        converter >> value;
                     }
                  };

                  template<>
                  struct format< string::Alignment::right>
                  {
                     // to string
                     void operator() ( const char* value, stream::Output::view_type destination, char padding)
                     {
                        const auto size = std::min( destination.size, static_cast< long>( std::strlen( value)));
                        auto value_first = std::begin( destination) + ( destination.size - size);

                        // pad the beginning, if any
                        std::fill( std::begin( destination), value_first, padding);

                        std::copy_n( value, size, value_first);
                     }

                     // to string
                     template< std::integral T>
                     auto operator() ( T value, stream::Output::view_type destination, char padding)
                     {
                        auto string = std::to_string( value);
                        operator()( string.c_str(), destination, padding);
                     }

                     // from string
                     void operator() ( stream::Input::view_type source, std::string& value, char padding)
                     {
                        auto first = std::find_if( std::begin( source), std::end( source), [padding]( auto c){ return c != padding;});
                        value.resize( std::distance( first, std::end( source)));
                        std::copy( first, std::end( source), std::begin( value));
                     }

                     // from string
                     template< std::integral T>
                     auto operator() ( stream::Input::view_type source, T& value, char padding)
                     {
                        std::string string;
                        operator()( source, string, padding);
                        std::istringstream converter{ std::move( string)};
                        converter >> value;
                     }
                  };

                  
               } // detail

               template< long field, string::Alignment alignment>
               struct format
               {
                  static void string( field::stream::Input& buffer, stream::Output& string, long size, char padding)
                  {
                     auto view = string.consume( size);
                     
                     try 
                     {
                        auto value = buffer.get< field>();
                        detail::format< alignment>{}( value, view, padding);
                     }
                     catch( const std::out_of_range&)
                     {
                        // field is not found.
                        // fill the view with padding, should become a _memset_
                        std::fill( std::begin( view), std::end( view), padding);
                     }
                  }

                  static void string( stream::Input& string, field::stream::Output& buffer, long size, char padding)
                  {
                     using traits = field::detail::field_traits< field>;
                     typename traits::format_value_type value{};

                     auto view = traits::consume( string, size);

                     detail::format< alignment>{}( view, value, padding);
                     buffer.add< field>( std::move( value));
                  }
               };

               struct Convert
               {
                  using callback_to_t = std::function< void( field::stream::Input&, stream::Output&)>;
                  using callback_from_t = std::function< void( stream::Input&, field::stream::Output&)>;

                  inline Convert( callback_to_t to, callback_from_t from)
                     : to{ std::move( to)}, from{ std::move( from)} {}

                  //! generic callback that can handle both from-string and to-string
                  template< typename Callback>
                  Convert( Callback&& callback) 
                     : to{ [callback]( field::stream::Input& input, stream::Output& output){ return callback( input, output);}},
                     from{ [callback]( stream::Input& input, field::stream::Output& output){ callback( input, output);}}
                  {}

                  callback_to_t to;
                  callback_from_t from;
               };

               namespace convert
               {
                  bool registration( std::string key, Convert action);

                  void from( const std::string& key, stream::Input string, char** buffer);
                  stream::Output to( const std::string& key, const char* buffer, stream::Output string);
                  
               } // convert
               

            } // string
         } // field
      } // internal
   } // buffer
} // casual

