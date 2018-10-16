#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>

namespace HTTP_PARSER
{
    #include "http_parser.h"
}

template<typename msg_type>
class HttpParser;

class HttpRequest;
class HttpResponse;


/**
 * A wrapper around http_parser for HTTP Request.
 */
template<>
class HttpParser<HttpRequest>
{
    HTTP_PARSER::http_parser parser;
    HTTP_PARSER::http_parser_settings setting;

    static int on_url(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_header_field(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_header_value(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_headers_complete(HTTP_PARSER::http_parser* parser);
    /**
     * Callback invoked by http_parser.
     * May not be called if request method does not signal a body, and no chunked encoding or
     * Content-Length is not presented in the headers.
     */
    static int on_body(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_message_complete(HTTP_PARSER::http_parser* parser);

/**
 * Since parser accept partial request as input
 * A State machine is needed, in case a field is separated by 2 input buffer.
 * Used to store the previous callback.
 */
    enum CallBack
    {
        None, Url, HeaderField, HeaderValue, HeaderComplete, Body,
    };

/**
 */
    enum State
    {
        FirstCall, AfterFirst,
    };

/**
 * Alternative of the std::string_view.
 * 
 * std::string_view is based on const char*, used with growing std::string,
 * might cause dangling pointer since the std::string could reallocate.
 * 
 * Used during parsing of the msg, use index to indicate the begining of
 * the string instead of using ptr. dangling string_view can be avoided.
 * 
 * All str_view_t will be converted to std::string_view after the std::string
 * it referenced to stop changing (on_headers_complete, and on_message_complete).
 */
    struct str_view_t
    {
        // index of begining
        size_t index;
        // length
        size_t len;
        std::string_view to_string_view(const char *ptr)
        { return std::string_view(ptr + this->index, this->len); }
    };

/**
 * Since callback invoked by http_parser needs to be static, and multiple parser
 * instance might be spined for a multithreaded setup, each parser need to have
 * its thread local storage/buffer.
 */
    struct instance_data_t
    {
        str_view_t url;
        size_t url_index;
        size_t url_len;
        //str_view_t last_header;
        size_t last_header_index;
        size_t last_header_len;
        //str_view_t header_value;
        size_t header_value_index;
        size_t header_value_len;

        CallBack last_callback;
        State state;
        std::vector<str_view_t> headers;
        std::unique_ptr<HttpRequest> req_ptr;
        bool complete;
    };
    instance_data_t data;

public:
    HttpParser();
    /**
     * Called before parsing each msg.
     */
    void init();
    bool parse(const std::string_view &);
    /**
     * Check if parsing of a message has ended (Whether or not msg
     * received is a complete msg)
     */
    bool complete();
    std::optional<HttpRequest*> result();
};


/**
 * A wrapper around http_parser for HTTP Response.
 */
template<>
class HttpParser<HttpResponse>
{
    HTTP_PARSER::http_parser parser;
    HTTP_PARSER::http_parser_settings setting;

    static int on_status(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_header_field(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_header_value(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_headers_complete(HTTP_PARSER::http_parser* parser);
    /**
     * Callback invoked by http_parser.
     * May not be called if request method does not signal a body, and no chunked encoding or
     * Content-Length is not presented in the headers.
     */
    static int on_body(HTTP_PARSER::http_parser* parser, const char *at, size_t length);
    static int on_message_complete(HTTP_PARSER::http_parser* parser);

/**
 * Since parser accept partial request as input
 * A State machine is needed, in case a field is separated by 2 input buffer.
 * Used to store the previous callback.
 */
    enum CallBack
    {
        None, Status, HeaderField, HeaderValue, HeaderComplete, Body,
    };

/**
 */
    enum State
    {
        FirstCall, AfterFirst,
    };

/**
 * Alternative of the std::string_view.
 * 
 * std::string_view is based on const char*, used with growing std::string,
 * might cause dangling pointer since the std::string could reallocate.
 * 
 * Used during parsing of the msg, use index to indicate the begining of
 * the string instead of using ptr. dangling string_view can be avoided.
 * 
 * All str_view_t will be converted to std::string_view after the std::string
 * it referenced to stop changing (on_headers_complete, and on_message_complete).
 */
    struct str_view_t
    {
        // index of begining
        size_t index;
        // length
        size_t len;
        std::string_view to_string_view(const char *ptr)
        { return std::string_view(ptr + this->index, this->len); }
    };

/**
 * Since callback invoked by http_parser needs to be static, and multiple parser
 * instance might be spined for a multithreaded setup, each parser need to have
 * its thread local storage/buffer.
 */
    struct instance_data_t
    {
        str_view_t status;
        size_t status_index;
        size_t status_len;
        //str_view_t last_header;
        size_t last_header_index;
        size_t last_header_len;
        //str_view_t header_value;
        size_t header_value_index;
        size_t header_value_len;

        CallBack last_callback;
        State state;
        std::vector<str_view_t> headers;
        std::unique_ptr<HttpResponse> resp_ptr;
        bool complete;
    };
    instance_data_t data;

public:
    HttpParser();
    void init();
    bool parse(const std::string_view &);
    bool complete();
    std::optional<HttpResponse*> result();
};


/**
 * immutable after constructed by the parser
 */
class HttpRequest
{
    using uint = unsigned int;

    uint method_num;
    std::string headers_str;
    std::string body_str;
    /**
     * Reference to headers_str.
     */
    std::string_view url_view;
    /**
     * Both field and value reference to headers_str
     */
    std::map<std::string_view, std::string_view> headers;

    HttpRequest() {}
public:
    /**
     * Not copyable, only moveable.
     */
    HttpRequest(const HttpRequest&) = delete;
    /**
     * Moveable
     */
    HttpRequest(HttpRequest&&);

    uint method();
    std::string_view url();
    std::optional<std::string_view> header(const std::string &field);
    std::optional<std::string_view> header(const char *field, size_t len);
    std::optional<std::string_view> header(const std::string_view &field);
    std::optional<std::string_view> body();
    friend HttpParser<HttpRequest>;
};


/**
 * immutable after constructed by the parser
 */
class HttpResponse
{
    using uint = unsigned int;

    uint status_num;
    std::string headers_str;
    std::string body_str;
    std::map<std::string_view, std::string_view> headers;

    HttpResponse() {}
public:
    HttpResponse(const HttpRequest&) = delete;
    HttpResponse(HttpResponse&&);

    uint status();
    std::optional<std::string_view> header(const std::string &field);
    std::optional<std::string_view> header(const char *field, size_t len);
    std::optional<std::string_view> header(const std::string_view &field);
    std::optional<std::string_view> body();
    friend HttpParser<HttpResponse>;
};

