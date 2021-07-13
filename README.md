the source code contains three file: 
  cribl_server.cpp
  httprequestparser.h
  httpparser/request

build the code using the command: g++ -std=c++11 cribl_server.cpp -o server
and then run the executable as: ./server

Users can set the port number in the Cribl_client.cpp file, the default port number is: 8090.

A valid URI format is like this: localhost:8090/alternatives.log.1/10/usr
  Localhost: hostname
  8090: port_number
  alternatives.log.1: query file in /var/log directory
  10: event_num
  usr: filter keyword
