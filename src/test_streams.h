#ifndef TEST_STREAMS_H
#define TEST_STREAMS_H

const char *test_stream = 
	"GET /test HTTP/1.1\r\n"
	"User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"
	"Host: 0.0.0.0=5000\r\n"
	"Accept: */*\r\n"
	"\r\n"
	"HTTP/1.1 301 Moved Permanently\r\n"
        "Location: http://www.google.com/\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Date: Sun, 26 Apr 2009 11:11:49 GMT\r\n"
        "Expires: Tue, 26 May 2009 11:11:49 GMT\r\n"
        "X-$PrototypeBI-Version: 1.6.0.3\r\n" /* $ char in header field */
        "Cache-Control: public, max-age=2592000\r\n"
        "Server: gws\r\n"
        "Content-Length:  219  \r\n"
        "\r\n"
        "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"
        "<TITLE>301 Moved</TITLE></HEAD><BODY>\n"
        "<H1>301 Moved</H1>\n"
        "The document has moved\n"
        "<A HREF=\"http://www.google.com/\">here</A>.\r\n"
        "</BODY></HTML>\r\n"
        "GET /test HTTP/1.1\r\n"
        "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"
        "Host: 0.0.0.0=5000\r\n"
        "Accept: */*\r\n"
        "\r\n";

#endif /* TEST_STREAMS_H */