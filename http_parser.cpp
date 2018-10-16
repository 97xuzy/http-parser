#include "http_parser.hpp"
using std::string;
using std::string_view;
using std::unique_ptr;
using std::optional;
#include <iostream>
#include <cassert>


#define RequestParser HttpParser<HttpRequest>
#define ResponseParser HttpParser<HttpResponse>


/**********************************************************************
 * 
 * HttpParser<HttpRequest>
 * 
 **********************************************************************/
RequestParser::HttpParser()
{
    HTTP_PARSER::http_parser_settings setting =
    {
        nullptr, // on_message_begin;
        on_url, // on_url;
        nullptr, // on_status;
        on_header_field, // on_header_field;
        on_header_value, // on_header_value;
        on_headers_complete, // on_headers_complete;
        on_body, // on_body;
        on_message_complete, // on_message_complete;
        nullptr, // on_chunk_header;
        nullptr // on_chunk_complete;
    };
    this->setting = setting;

    this->data.complete = false;
    this->parser.data = &this->data;

    http_parser_init(&this->parser, HTTP_PARSER::HTTP_REQUEST);
}

int RequestParser::on_url(HTTP_PARSER::http_parser *parser, const char *at, size_t length)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpRequest *req = data->req_ptr.get();

    if(data->last_callback != Url)
    {
        data->state = FirstCall;
    }
    else
    {
        data->state = AfterFirst;
    }

    if(data->state == FirstCall)
    {
        // method
        req->method_num = parser->method;

        // url
        data->url_index= req->headers_str.length();
        data->url_len = length;
    }
    else
    {
        data->url_len += length;
    }

    req->headers_str.append(string(at, length));

    data->last_callback = Url;
    return 0;
}

int RequestParser::on_header_field(HTTP_PARSER::http_parser* parser, const char *at, size_t length)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpRequest *req = data->req_ptr.get();

    switch(data->last_callback)
    {
        case HeaderField:
            data->state = AfterFirst;
            break;
        case Url:
            data->state = FirstCall;
            break;
        case HeaderValue:
            data->state = FirstCall;
            // Create str_view_t for last header value, and insert into std::map
            data->headers.push_back({data->header_value_index, data->header_value_len});
            break;
        default:
            assert(false);
    }

    if(data->state == FirstCall)
    {
        data->last_header_index = req->headers_str.length();
        data->last_header_len = length;
    }
    else
    {
        data->last_header_len += length;
    }

    req->headers_str.append(string(at, length));

    data->last_callback = HeaderField;
    return 0;
}

int RequestParser::on_header_value(HTTP_PARSER::http_parser* parser, const char *at, size_t length)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpRequest *req = data->req_ptr.get();

    if(data->last_callback != HeaderValue)
    {
        data->state = FirstCall;
        assert(data->last_callback == HeaderField);
        // Create string_view for the field corrsponding with this value
        data->headers.push_back({data->last_header_index, data->last_header_len});
    }
    else
    {
        data->state = AfterFirst;
    }

    if(data->state == FirstCall)
    {
        data->header_value_index = req->headers_str.length();
        data->header_value_len = length;
    }
    else
    {
        data->header_value_len += length;
    }

    req->headers_str.append(string(at, length));

    data->last_callback = HeaderValue;
    return 0;
}

/**
 * Construct string_view from str_view_t for all headers, and url.
 * since all headers, and url are stored in HttpRequest::headers_str, and headers_str
 * will not be changed after this point.
 */
int RequestParser::on_headers_complete(HTTP_PARSER::http_parser *parser)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpRequest *req = data->req_ptr.get();

    if(data->last_callback == HeaderValue)
    {
        // Create string_view for last header value, and insert into std::map
        data->headers.push_back({data->header_value_index, data->header_value_len});

        // length must be even number, {field, value} pairs
        assert(data->headers.size() % 2 == 0);

        for(size_t i = 0; i < data->headers.size(); i+=2)
        {
            string_view field = data->headers.at(i).to_string_view(&req->headers_str[0]);
            string_view value = data->headers.at(i+1).to_string_view(&req->headers_str[0]);
            req->headers.emplace(field, value);
        }

        data->headers.clear();
    }
    // no headers are presented in the request
    else
    {
    }

    // Construct string_view for url
    data->url = {data->url_index, data->url_len};
    req->url_view = data->url.to_string_view(&req->headers_str[0]);

    //reset_after_header_pair((instance_data_t*)parser->data);
    data->last_callback = HeaderComplete;
    return 0;
}

int RequestParser::on_body(HTTP_PARSER::http_parser* parser, const char *at, size_t length)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpRequest *req = data->req_ptr.get();

    req->body_str.append(string(at, length));
    return 0;
}

int RequestParser::on_message_complete(HTTP_PARSER::http_parser* parser)
{
    (void)parser;
    instance_data_t *data = (instance_data_t*)parser->data;
    //HttpRequest *req = data->req_ptr.get();
    data->complete = true;

    // Clear temp data
    data->headers.clear();

    return 0;
}

void RequestParser::init()
{
    http_parser_init(&this->parser, HTTP_PARSER::HTTP_REQUEST);

    this->data.url_index = 0;
    this->data.url_len = 0;
    this->data.last_header_index = 0;
    this->data.last_header_len = 0;
    this->data.header_value_index = 0;
    this->data.header_value_len = 0;

    this->data.last_callback = None;
    this->data.state = FirstCall;

    this->data.complete = false;

    this->data.req_ptr = std::unique_ptr<HttpRequest>(new HttpRequest());
}

bool RequestParser::parse(const string_view &input)
{
    this->data.complete = false;
    size_t nparsed = http_parser_execute(&this->parser, &this->setting, input.data(), input.length());

    return nparsed == input.length();
}

bool RequestParser::complete()
{
    return this->data.complete;
}

std::optional<HttpRequest*> RequestParser::result()
{
    // signal EOF to the parser
    http_parser_execute(&this->parser, &this->setting, nullptr, 0);

    // If called when the full msg has not been feed into parser (or msg has error, and
    // doesn't terminate) , return nullopt
    if(! this->data.complete)
        return std::nullopt;

    HttpRequest *ptr = this->data.req_ptr.get();
    this->data.req_ptr.release();
    return ptr;
}

/**********************************************************************
 * 
 * HttpParser<HttpResponse>
 * 
 **********************************************************************/
ResponseParser::HttpParser()
{
    HTTP_PARSER::http_parser_settings setting =
    {
        nullptr, // on_message_begin;
        nullptr, // on_url;
        on_status, // on_status;
        on_header_field, // on_header_field;
        on_header_value, // on_header_value;
        on_headers_complete, // on_headers_complete;
        on_body, // on_body;
        on_message_complete, // on_message_complete;
        nullptr, // on_chunk_header;
        nullptr // on_chunk_complete;
    };
    this->setting = setting;

    this->data.complete = false;
    this->parser.data = &this->data;

    http_parser_init(&this->parser, HTTP_PARSER::HTTP_REQUEST);
}

int ResponseParser::on_status(HTTP_PARSER::http_parser *parser, const char *at, size_t length)
{
    (void)at;
    (void )length;
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpResponse *resp = data->resp_ptr.get();

    resp->status_num = parser->status_code;

    data->last_callback = Status;
    return 0;
}

int ResponseParser::on_header_field(HTTP_PARSER::http_parser* parser, const char *at, size_t length)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpResponse *resp = data->resp_ptr.get();

    switch(data->last_callback)
    {
        case HeaderField:
            data->state = AfterFirst;
            break;
        case Status:
            data->state = FirstCall;
            break;
        case HeaderValue:
            data->state = FirstCall;
            // Create str_view_t for last header value, and insert into std::map
            data->headers.push_back({data->header_value_index, data->header_value_len});
            break;
        default:
            assert(false);
    }

    if(data->state == FirstCall)
    {
        data->last_header_index = resp->headers_str.length();
        data->last_header_len = length;
    }
    else
    {
        data->last_header_len += length;
    }

    resp->headers_str.append(string(at, length));

    data->last_callback = HeaderField;
    return 0;
}

int ResponseParser::on_header_value(HTTP_PARSER::http_parser* parser, const char *at, size_t length)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpResponse *resp = data->resp_ptr.get();

    if(data->last_callback != HeaderValue)
    {
        data->state = FirstCall;
        assert(data->last_callback == HeaderField);
        // Create string_view for the field corrsponding with this value
        data->headers.push_back({data->last_header_index, data->last_header_len});
    }
    else
    {
        data->state = AfterFirst;
    }

    if(data->state == FirstCall)
    {
        data->header_value_index = resp->headers_str.length();
        data->header_value_len = length;
    }
    else
    {
        data->header_value_len += length;
    }

    resp->headers_str.append(string(at, length));

    data->last_callback = HeaderValue;
    return 0;
}

/**
 * Construct string_view from str_view_t for all headers, and url.
 * since all headers, and url are stored in HttpRequest::headers_str, and headers_str
 * will not be changed after this point.
 */
int ResponseParser::on_headers_complete(HTTP_PARSER::http_parser *parser)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpResponse *resp = data->resp_ptr.get();

    if(data->last_callback == HeaderValue)
    {
        // Create string_view for last header value, and insert into std::map
        data->headers.push_back({data->header_value_index, data->header_value_len});

        // length must be even number, {field, value} pairs
        assert(data->headers.size() % 2 == 0);

        for(size_t i = 0; i < data->headers.size(); i+=2)
        {
            string_view field = data->headers.at(i).to_string_view(&resp->headers_str[0]);
            string_view value = data->headers.at(i+1).to_string_view(&resp->headers_str[0]);
            resp->headers.emplace(field, value);
        }

        data->headers.clear();
    }
    // no headers are presented in the request
    else
    {
    }

    //reset_after_header_pair((instance_data_t*)parser->data);
    data->last_callback = HeaderComplete;
    return 0;
}

int ResponseParser::on_body(HTTP_PARSER::http_parser* parser, const char *at, size_t length)
{
    instance_data_t *data = (instance_data_t*)parser->data;
    HttpResponse *resp = data->resp_ptr.get();

    resp->body_str.append(string(at, length));
    return 0;
}

int ResponseParser::on_message_complete(HTTP_PARSER::http_parser* parser)
{
    (void)parser;
    instance_data_t *data = (instance_data_t*)parser->data;
    data->complete = true;
    return 0;
}

void ResponseParser::init()
{
    http_parser_init(&this->parser, HTTP_PARSER::HTTP_RESPONSE);

    this->data.last_header_index = 0;
    this->data.last_header_len = 0;
    this->data.header_value_index = 0;
    this->data.header_value_len = 0;

    this->data.last_callback = None;
    this->data.state = FirstCall;

    this->data.complete = false;

    this->data.resp_ptr = std::unique_ptr<HttpResponse>(new HttpResponse());
}

bool ResponseParser::parse(const string_view &input)
{
    this->data.complete = false;
    size_t nparsed = http_parser_execute(&this->parser, &this->setting, input.data(), input.length());

    return nparsed == input.length();
}

bool ResponseParser::complete()
{
    return this->data.complete;
}

std::optional<HttpResponse*> ResponseParser::result()
{
    // signal EOF to the parser
    http_parser_execute(&this->parser, &this->setting, nullptr, 0);

    this->data.headers.clear();

    HttpResponse *ptr = this->data.resp_ptr.get();
    this->data.resp_ptr.release();
    return ptr;
}


/**********************************************************************
 * 
 * HttpRequest
 * 
 **********************************************************************/
HttpRequest::HttpRequest(HttpRequest &&other)
{
    this->method_num = other.method_num;
    this->headers_str = std::move(other.headers_str);
    this->body_str = std::move(other.body_str);
    this->url_view = other.url_view;
    this->headers = std::move(other.headers);
}

string_view HttpRequest::url()
{
    return this->url_view;
}

unsigned int HttpRequest::method()
{
    return this->method_num;
}

optional<string_view> HttpRequest::header(const std::string &field)
{
    if(this->headers.count(string_view(&field[0], field.length())) == 0)
        return std::nullopt;
    return this->headers.at(string_view(&field[0], field.length()));
}

optional<string_view> HttpRequest::header(const char *field, size_t len)
{
    if(this->headers.count(string_view(field, len)) == 0)
        return std::nullopt;
    return this->headers.at(string_view(field, len));
}

optional<string_view> HttpRequest::header(const string_view &field)
{
    if(this->headers.count(field) == 0)
        return std::nullopt;
    return this->headers.at(field);
}

optional<string_view> HttpRequest::body()
{
    return string_view(&this->body_str[0], this->body_str.length());
}


/**********************************************************************
 * 
 * HttpResponse
 * 
 **********************************************************************/
HttpResponse::HttpResponse(HttpResponse &&other)
{
    this->status_num = other.status_num;
    this->headers_str = std::move(other.headers_str);
    this->body_str = std::move(other.body_str);
    this->headers = std::move(other.headers);
}

unsigned int HttpResponse::status()
{
    return this->status_num;
}

optional<string_view> HttpResponse::header(const std::string &field)
{
    if(this->headers.count(string_view(&field[0], field.length())) == 0)
        return std::nullopt;
    return this->headers.at(string_view(&field[0], field.length()));
}

optional<string_view> HttpResponse::header(const char *field, size_t len)
{
    if(this->headers.count(string_view(field, len)) == 0)
        return std::nullopt;
    return this->headers.at(string_view(field, len));
}

optional<string_view> HttpResponse::header(const string_view &field)
{
    if(this->headers.count(field) == 0)
        return std::nullopt;
    return this->headers.at(field);
}

optional<string_view> HttpResponse::body()
{
    return string_view(&this->body_str[0], this->body_str.length());
}


