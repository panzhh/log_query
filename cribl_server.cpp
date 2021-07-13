#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <iterator>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <ctime>

#include "request.h"
#include "httprequestparser.h"
#define PORT 8090

// execute shell command and return the results as string.
std::string exec(const char* cmd)
 {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

    if (!pipe) 
    {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
    {
        result += buffer.data();
    }

    return result;
}

// send the response string by the socket_id
void send_response(int socket_id, const std::string& response)
{
    if (send(socket_id, response.data(), response.size(), 0) < 0)
    {
        std::cout << "------------------response message sent failed.-----------\n";
    }
    else
    {
        std::cout << "------------------response message sent-------------------\n";
    }

    close(socket_id);
}


bool fileExists( const std::string &Filename )
{
    return access( Filename.c_str(), 0 ) == 0;
}

bool isNumber(const std::string& s)
{
    if (s.empty())
    {
        return false;
    }

    for (int i = 0; i < s.length(); i++)
    {
        if (!isdigit(s[i]))
        {
            return false;
        }
    }
 
    return true;
}



int main(int argc, char const *argv[])
{
    int server_fd, new_socket; long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    std::memset(address.sin_zero, '\0', sizeof address.sin_zero);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }

    // prepare response message
    std::string resp = "";
    const std::string h = "HTTP/1.1 200 OK";
    const std::string s = "Server: Muffin 1.0\r\n";
    std::string t;
    std::string content;
    std::string length;
    std::string type = "Content-Type: text/html\r\n";

    while ( true )
    {
        std::cout << "\n================ Waiting for new connection =============\n\n";

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        std::vector<char> buffer(1024, 0);
        size_t offset = 0;

        // read the socket to buffer
        do
        {
            valread = read( new_socket , buffer.data() + offset, 1024);
            offset += valread;
        } while ( valread == 1024 );

        std::string http_request(buffer.begin(), buffer.end());
        std::cout << "Received the http_resquest is: \n" << http_request << std::endl;

        // check if the request uri format is valid.
        httpparser::Request request;
        httpparser::HttpRequestParser parser;
        httpparser::HttpRequestParser::ParseResult check_request = parser.parse(request, 
                                                                      http_request.data(), 
                                                                      http_request.data() + http_request.size());
    
        if( check_request != httpparser::HttpRequestParser::ParsingCompleted )
        {
            std::cerr << "Parsing failed" << std::endl;
            content = "Invalid request.";
            length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
            std::time_t now = std::time(0);
            t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
            resp = h + t + s + length + type + "\n" + content + "\r\n";
            send_response(new_socket, resp);
            continue;
        }

        std::cout << request.inspect() << std::endl;
        std::cout << "method: " << request.method << std::endl;
        std::cout << "uri: " << request.uri << std::endl;
        std::string& uri = request.uri;

        if (request.uri.empty())
        {
            content = "Invalid uri, uri can not be empty.";
            length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
            std::time_t now = std::time(0);
            t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
            resp = h + t + s + length + type + "\n" + content + "\r\n";
            send_response(new_socket, resp);
            continue;
        }

        if (uri[uri.size()-1] != '/')
        {
            uri += '/';
        }

        std::vector<std::string> items(3); 
        std::string& file_name = items[0];
        std::string& event_num = items[1];
        std::string& keyword = items[2];

        int slash_cnt = 0;
        for (size_t i = 0; i < uri.size(); ++i)
        {
            //std::cout << "uri[" << i << "] is " << uri[i] << std::endl;
            if (uri[i] == '/')
            {
                slash_cnt++;
            }
            else 
            {
                if (slash_cnt > 3)
                {
                    content = "Invalid uri. There are two many slashes in the uri.";
                    length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
                    std::time_t now = std::time(0);
                    t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
                    resp = h + t + s + length + type + "\n" + content + "\r\n";
                    std::cerr << "program exits, Invalide uri " << uri << std::endl;
                    send_response(new_socket, resp);
                    continue;
                }

                items[slash_cnt-1] += uri[i];
            }
        }

        // check query file, check if /var/log exists

        if ( !fileExists("/var/log/") )
        {
            content = "/var/log does not exist in the server.";
            length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
            std::time_t now = std::time(0);
            t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
            resp = h + t + s + length + type + "\n" + content + "\r\n";
            std::cerr << "program exits: /var/log directory does exist in the server.";
            send_response(new_socket, resp);
            continue;
        }

        if (file_name.empty())
        {
            content = "Invalid uri. query file name can not be empty.";
            length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
            std::time_t now = std::time(0);
            t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
            resp = h + t + s + length + type + "\n" + content + "\r\n";
            std::cerr << "program exits: query file can not be empty.\n";
            send_response(new_socket, resp);
            continue;
        }
        else if ( !fileExists("/var/log/" + file_name) )
        {
            content = "Invalid uri. query file name '" + file_name + "' does not exist in /var/log directory.";
            length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
            std::time_t now = std::time(0);
            t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
            resp = h + t + s + length + type + "\n" + content + "\r\n";
            std::cout << "program exits: /var/log directory does not contain a file named " << file_name << "\n";
            send_response(new_socket, resp);
            continue;
        }

        // check event number n is valid

        if (!isNumber(event_num))
        {
            content = "Invalid uri: '" + event_num + "' is not a valid postive integer.";
            length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
            std::time_t now = std::time(0);
            t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
            resp = h + t + s + length + type + "\n" + content + "\r\n";
            std::cout << "program exits: " << event_num << " is not a valid positive integer.\n";
            send_response(new_socket, resp);
            continue;
        }
        else
        {
            int n = std::stoi(event_num);

            if ( n < 1 )
            {
                content = "Invalid uri: event_number '" + event_num + "' must be a postive integer.";
                length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
                std::time_t now = std::time(0);
                t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
                resp = h + t + s + length + type + "\n" + content + "\r\n";
                std::cout << "program exits: event_number " << event_num << " must be a postive number.\n";
                std::cout << "\n\n" << resp << "\n\n";
                send_response(new_socket, resp);
                continue;
            }
        }

        // check the keywords
        if (keyword.empty())
        {
            content = "Invalid uri: query keyword can not be empty.";
            length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
            std::time_t now = std::time(0);
            t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
            resp = h + t + s + length + type + "\n" + content + "\r\n";
            std::cout << "program exits: query keyword can not be empty.\n";
            send_response(new_socket, resp);
            continue;
        }
        
        /*
        std::cout << "Received query uri: \n";
        for (size_t i = 0; i<items.size(); ++i)
        {
            std::cout << "query_item[" << i << "] is: " << items[i] << std::endl;
        }
        */

        // assemble query cmd
        std::stringstream ss;
        ss << "grep " << keyword << " /var/log/" << file_name 
           << " | sort -r | " << "tail -" << event_num;

        std::string cmd(ss.str());
        std::cout << "Query cmd: '" << cmd << "'.\n";
        content = exec(cmd.c_str());
        length = "Content-Length: " + std::to_string(content.size()) + "\r\n";
        std::time_t now = std::time(0);
        t = "Date: " + std::string(std::ctime(&now)) + "\r\n";
        resp = h + t + s + length + type + "\n" + content + "\r\n";
        std::cout << "Query results are:\n\n" << content << "\n\n";
        send_response(new_socket, resp);
    }

    return 0;
}
