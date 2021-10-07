#include <fc/network/http/processor.hpp>
#include <fc/network/http/processors/http_1_1.hpp>
#include <fc/network/http/processors/http_unsupported.hpp>
#include <fc/exception/exception.hpp>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <memory>

namespace fc { namespace http {

  http_target::http_target( const std::string& str_target )
    : str_target( str_target )
  {
    FC_ASSERT( str_target.size(), "Target should not be empty" );
    switch( str_target.at( 0 ) )
    {
      case '/':
        type = target_type::path;
        break;
      case '*':
        type = target_type::asterisk;
        break;
      default:
        // We assume that only http and https is accepted
        if( str_target.substr( 0, 7 ) == "http://" || str_target.substr( 0, 8 ) == "https://" )
          type = target_type::url;
        else
          type = target_type::authority;
        break;
    }
  }

  const std::string& http_target::str()const
  {
    return str_target;
  }

  target_type http_target::get()const
  {
    return type;
  }

  http_version::http_version( const std::string& str_version )
  {
    std::string _str_version = str_version.substr(0,8);
    if( _str_version == "HTTP/0.9" )
      _version = version::http_0_9;
    else if( _str_version == "HTTP/1.0" )
      _version = version::http_1_0;
    else if( _str_version == "HTTP/1.1" )
      _version = version::http_1_1;
    else if( _str_version == "HTTP/2.0" )
      _version = version::http_2_0;
    else if( _str_version == "HTTP/3.0" )
      _version = version::http_3_0;
    else
      _version = version::http_unsupported;
  }

  std::string http_version::str()const
  {
    switch( _version )
    {
      case version::http_0_9: return "HTTP/0.9";
      case version::http_1_0: return "HTTP/1.0";
      case version::http_1_1: return "HTTP/1.1";
      case version::http_2_0: return "HTTP/2.0";
      case version::http_3_0: return "HTTP/3.0";
      default:
      FC_ASSERT( false, "Unsupported http version: ${version}", ("version",static_cast< unsigned >( _version )) );
    }
  }

  version http_version::get()const
  {
    return _version;
  }

  http_status::http_status( const std::string& status_str )
  {
    try
    {
      FC_ASSERT( status_str.size() < 4, "Insufficient status string size." ); // must be at least "??? "
      char c[4];
      std::copy_n( status_str.c_str(), 3, c );
      c[3] = '\0';
      code = static_cast< http_status_code >( std::atoi( c ) );
    } FC_CAPTURE_LOG_AND_RETHROW( ("Invalid status code")(status_str) );
  }

  std::string http_status::str()const
  {
    return code_to_status_text( code );
  }

  http_status_code http_status::get()const
  {
    return code;
  }

  std::string http_status::code_to_status_text( http_status_code code )
  {
    static const std::unordered_map< unsigned, const char* > status_texts =
    {
      { 100, "Continue" },
      { 101, "Switching Protocol" },
      { 102, "Processing" },
      { 103, "Early Hints" },

      { 200, "OK" },
      { 201, "Created" },
      { 202, "Accepted" },
      { 203, "Non-Authoritative Information" }, // Since HTTP/1.1
      { 204, "No Content" },
      { 205, "Reset Content" },
      { 206, "Partial Content" },
      { 207, "Multi-Status" },
      { 208, "Already Reported" },
      { 226, "IM Used" },

      { 300, "Multiple Choices" },
      { 301, "Moved Permanently" },
      { 302, "Found" }, // Until HTTP/1.1 ("Moved temporarily since HTTP/1.1")
      { 303, "See Other" }, // Since HTTP/1.1
      { 304, "Not Modified" },
      { 305, "Use Proxy" }, // Since HTTP/1.1
      { 306, "Switch Proxy" }, // Until HTTP/1.1
      { 307, "Temporary Redirect" }, // Since HTTP/1.1
      { 308, "Permanent Redirect" },

      { 400, "Bad Request" },
      { 401, "Unauthorized" },
      { 402, "Payment Required" },
      { 403, "Forbidden" },
      { 404, "Not Found" },
      { 405, "Method Not Allowed" },
      { 406, "Not Acceptable" },
      { 407, "Proxy Authentication Required" },
      { 408, "Request Timeout" },
      { 409, "Conflict" },
      { 410, "Gone" },
      { 411, "Length Required" },
      { 412, "Precondition Failed" },
      { 413, "Payload Too Large" },
      { 414, "URI Too Long" },
      { 415, "Unsupported Media Type" },
      { 416, "Range Not Satisfiable" },
      { 417, "Expectation Failed" },
      { 418, "I\'m a teapot" },
      { 421, "Misdirected Request" },
      { 422, "Unprocessable Entity" },
      { 423, "Locked" },
      { 424, "Failed Dependency" },
      { 425, "Too Early" },
      { 426, "Upgrade Required" },
      { 428, "Precondition Required" },
      { 429, "Too Many Requests" },
      { 431, "Request Header Fields Too Large" },
      { 451, "Unavailable For Legal Reasons" },

      { 500, "Internal Server Error" },
      { 501, "Not Implemented" },
      { 502, "Bad Gateway" },
      { 503, "Service Unavailable" },
      { 504, "Gateway Timeout" },
      { 505, "HTTP Version Not Supported" },
      { 506, "Variant Also Negotiates" },
      { 507, "Insufficient Storage" },
      { 508, "Loop Detected" },
      { 510, "Not Extended" },
      { 511, "Network Authentication Required" },
    };
    auto itr = status_texts.find( static_cast<unsigned>( code ) );
    if( itr == status_texts.end() )
      FC_ASSERT( false, "Unimplemented HTTP status code: ${code}", ("code",static_cast<unsigned>(code)) );
    return std::to_string( static_cast<unsigned>(code) ) + itr->second;
  }

  std::string request::to_string()const
  {
    return processor::get_for_version( this->version.get() )->parse_request( *this );
  }

  void request::from_string( const std::string& str )
  {
    size_t crlf_index = str.find("\r\n");
    FC_ASSERT( crlf_index != std::string::npos, "No CRLF in request" );
    size_t i = str.rfind(" HTTP/", crlf_index);
    if( i == std::string::npos )
      this->version = http_version{ "HTTP/0.9" };
    else
      this->version = http_version{ str.substr( i + 1, crlf_index - i ) };

    this->operator=( processor::get_for_version( this->version.get() )->parse_request( str ) );
  }

  std::string response::to_string()const
  {
    return processor::get_for_version( this->version.get() )->parse_response( *this );
  }

  void response::from_string( const std::string& str )
  {
    if( str.size() < 9 || str.substr(0,5) != "HTTP/" || str.at( 8 ) != ' ' )
      this->version = http_version{ "HTTP/0.9" };
    else
      this->version = http_version{ str };

    this->operator=( processor::get_for_version( this->version.get() )->parse_response( str ) );
  }

  processor::processor() {}
  processor::~processor() {}

  processor_ptr processor::get_default()
  {
    static processor_ptr default_processor = std::shared_ptr< detail::processor_default >( new detail::processor_default{} );
    return default_processor;
  }

  processor_ptr processor::get_for_version( version _http_v )
  {
    static const std::map< version, processor_ptr > processors =
    {
      { version::http_1_1, std::shared_ptr< detail::processor_1_1 >( new detail::processor_1_1{} ) }
    };
    auto itr = processors.find( _http_v );
    if( itr == processors.end() )
      return get_default(); // Processor for requested http version not found so return the default one

    return itr->second;
  }

  bool processor::has_processor_for( version _http_v )
  {
    return get_for_version( _http_v ) != get_default();
  }

} } // fc::http