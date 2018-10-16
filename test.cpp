#include "http_parser.hpp"
#include <string>
#include <string_view>
#include <iostream>
#include <optional>
#include <string_view>
#include <cstring>
#include <cassert>

using namespace std;

/*
constexpr char req[] = "PUT /test.com/test1 HTTP/1.1\r\n"
    "Host: test.com\r\n"
    "Field_1: value111\r\n"
    "Field_2: value222\r\n"
    "Field_3-----------------------------------------------: value333_-------------------------------------------------------\r\n"
    "\r\n"
    "body_+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    "\r\n";
*/

string read_n_from(const string& input, size_t n);
size_t read(string& fd, size_t &index, void *buf, size_t count);

bool request_test1();
bool request_test2();
bool request_test3();

bool response_test1();
bool response_test2();
bool response_test3();

int main()
{

    request_test1();
    request_test2();
    request_test3();

    response_test1();
    response_test2();
    response_test3();

    return 0;
}


bool request_test1()
{
    constexpr char req[] = "PUT /test.com/test1 HTTP/1.1\r\n"
        "Host: test.com\r\n"
        "Content-Length: 40\r\n"
        "Field-1AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA: BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\r\n"
        "\r\n"
        "0123456789012345678901234567890123456789\r\n";

    HttpRequest *ptr;
    string request(req);
    {
        HttpParser<HttpRequest> parser;

        parser.init();

        size_t index = 0;
        string buffer(10, 0);
        size_t byte_read = 0;
        while((byte_read = read(request, index, &buffer[0], 10)) > 0
            && !parser.complete())
        {
            assert(parser.parse(string_view(&buffer[0], byte_read)));
        }
        ptr = parser.result().value();
    }

    assert(ptr->method() == (int)HTTP_PARSER::HTTP_PUT);

    assert(ptr->url().compare("/test.com/test1") == 0);

    assert(ptr->header(string("Host")).value().compare("test.com") == 0);

    assert(ptr->header(string("Content-Length")).value().compare("40") == 0);

    assert(ptr->header(string("Field-1AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")).value().compare("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB") == 0);

    assert(ptr->body().value().compare("0123456789012345678901234567890123456789") == 0);

    delete ptr;

    return true;
}

bool request_test2()
{
    constexpr char req[] = "GET / HTTP/1.1\r\n"
        "Host: test.com\r\n"
        "Field-1CCCCCCCCCAAAAAAAAAAAAAAAAAAAAA: BBBBBBBBBBBBBCCCCCCCCCCCCCCCBBBBBBBBBBBBBBBBB\r\n"
        "\r\n";

    HttpRequest *ptr;
    string request(req);
    {
        HttpParser<HttpRequest> parser;

        parser.init();

        size_t index = 0;
        string buffer(10, 0);
        size_t byte_read = 0;
        while((byte_read = read(request, index, &buffer[0], 10)) > 0)
        {
            assert(parser.parse(string_view(&buffer[0], byte_read)));
        }
        ptr = parser.result().value();
    }

    assert(ptr->method() == (int)HTTP_PARSER::HTTP_GET);

    assert(ptr->url().compare("/") == 0);

    assert(ptr->header(string("Host")).value().compare("test.com") == 0);

    assert(ptr->header(string("Field-1CCCCCCCCCAAAAAAAAAAAAAAAAAAAAA")).value().compare("BBBBBBBBBBBBBCCCCCCCCCCCCCCCBBBBBBBBBBBBBBBBB") == 0);

    assert(ptr->body().value().empty());

    delete ptr;

    return true;
}

bool request_test3()
{
    constexpr char req[] = "GET / HTTP/1.1\r\n"
        "\r\n";

    HttpRequest *ptr = nullptr;
    string request(req);
    {
        HttpParser<HttpRequest> parser;

        parser.init();

        size_t index = 0;
        string buffer(10, 0);
        size_t byte_read = 0;
        while((byte_read = read(request, index, &buffer[0], 10)) > 0)
        {
            assert(parser.parse(string_view(&buffer[0], byte_read)));
        }
        ptr = parser.result().value_or(nullptr);
        assert(parser.complete());
        assert(ptr != nullptr);
    }

    assert(ptr->method() == (int)HTTP_PARSER::HTTP_GET);

    assert(ptr->url().compare("/") == 0);

    //assert(ptr->header(string("Host")).value().compare("test.com") == 0);

    // body should be empty
    assert(ptr->body().value().empty());

    delete ptr;

    return true;
}

bool response_test1()
{
    constexpr char resp[] = "HTTP/1.1 200 OK\r\n"
        "\r\n";

    HttpResponse *ptr;
    string response(resp);
    {
        HttpParser<HttpResponse> parser;

        parser.init();

        size_t index = 0;
        string buffer(10, 0);
        size_t byte_read = 0;
        while((byte_read = read(response, index, &buffer[0], 10)) > 0)
        {
            assert(parser.parse(string_view(&buffer[0], byte_read)));
        }
        ptr = parser.result().value();
    }

    assert(ptr->status() == (int)HTTP_PARSER::HTTP_STATUS_OK);

    // body should be empty
    assert(ptr->body().value().empty());

    delete ptr;

    return true;
}

bool response_test2()
{
    constexpr char resp[] = "HTTP/1.1 200 OK\r\n"
        "Date: Mon, 18 Jul 2016 16:06:00 GMT\r\n"
        "\r\n";

    HttpResponse *ptr;
    string response(resp);
    {
        HttpParser<HttpResponse> parser;

        parser.init();

        size_t index = 0;
        string buffer(10, 0);
        size_t byte_read = 0;
        while((byte_read = read(response, index, &buffer[0], 10)) > 0)
        {
            assert(parser.parse(string_view(&buffer[0], byte_read)));
        }
        ptr = parser.result().value();
    }

    assert(ptr->status() == (int)HTTP_PARSER::HTTP_STATUS_OK);

    assert(ptr->header(string("Date")).value().compare("Mon, 18 Jul 2016 16:06:00 GMT") == 0);

    // body should be empty
    assert(ptr->body().value().empty());

    delete ptr;

    return true;
}

bool response_test3()
{
    constexpr char resp[] = "HTTP/1.1 200 OK\r\n"
        "Date: Mon, 18 Jul 2016 16:06:00 GMT\r\n"
        "\r\n"
        "JKGjhd dfsgfbsdhladksgmhdsjkf\nfsdafnghdjg\nfsdafas\r\n";

    HttpResponse *ptr;
    string response(resp);
    {
        HttpParser<HttpResponse> parser;

        parser.init();

        size_t index = 0;
        string buffer(10, 0);
        size_t byte_read = 0;
        while((byte_read = read(response, index, &buffer[0], 10)) > 0)
        {
            assert(parser.parse(string_view(&buffer[0], byte_read)));
        }
        ptr = parser.result().value();
    }

    assert(ptr->status() == (int)HTTP_PARSER::HTTP_STATUS_OK);

    assert(ptr->header(string("Date")).value().compare("Mon, 18 Jul 2016 16:06:00 GMT") == 0);

    assert(ptr->body().value().compare("JKGjhd dfsgfbsdhladksgmhdsjkf\nfsdafnghdjg\nfsdafas") == 0);

    // body should be empty
    assert(ptr->body().value().empty());

    delete ptr;

    return true;
}

string read_n_from(const string& input, size_t n)
{
    static size_t index = 0;

    if(index < input.length())
    {
        index += n;
        if(index + n < input.length())
            return input.substr(index - n, n);
        else
            return input.substr(index - n);
    }
    return string();
}

size_t read(string& fd, size_t &index, void *buf, size_t count)
{
    if(index > fd.length()) return 0;
    size_t byte_read = 0;
    size_t len_remain = fd.length() - index;
    char *ptr = (char*)buf;
    byte_read = len_remain < count ? len_remain : count;

    memcpy(ptr, &fd[index], byte_read);
    index += byte_read;

    return byte_read;
}

