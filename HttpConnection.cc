/*
 * Copyright ©2022 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 595 for use solely during Spring Semester 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <cstdint>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;
using std::cout;
using std::endl;

namespace searchserver {

static const char *kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

  // Use "wrapped_read" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use parse_request()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes next_request()!

  // TODO: implement

bool HttpConnection::next_request(HttpRequest *request) {
    
  uint16_t index;
  // check to see if anything exists in buffer. check if content is already loaded and if a full request exists
  if(buffer_.find(kHeaderEnd) == string::npos) {
    while (1) {
      int res = wrapped_read(fd_,&buffer_);

      if (res == 0) { // checking EOF
        break; 
      } else if (res < 0) { // checking error
        return false;
      } else { // if header is found
        if(buffer_.find(kHeaderEnd) != string::npos) {
          break;
        }
        continue;
      }
    }
  }  
  index = buffer_.find(kHeaderEnd);
  if (index == string::npos) {
    return false;
  }
  string temp = buffer_.substr(0,index);
  parse_request(temp, request);
  buffer_.erase(0,index + kHeaderEndLen);
  return true;
}
      

bool HttpConnection::write_response(const HttpResponse &response) {
  // Implement so that the response is converted to a string
  // and written out to the socket for this connection
  // TODO: implement
  string output = response.GenerateResponseString();

  if (output.length() == 0) {
    return false;
  }
  
  while(1) {
    int wres = wrapped_write(fd_, output);

      if(wres  == -1) {
        if (errno == EINTR) {
          continue;
        }
          // close(fd_);
          return false;
      }
      break;
  }

  return true;
}

  // Split the request into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers_ (i.e., HttpRequest::AddHeader).  You should look
  // at HttpRequest.h for details about the HTTP header format that
  // you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.
  //
  // If a request is malformed, return false, otherwise true and 
  // the parsed request is returned via *out
  
  // TODO: implement

bool HttpConnection::parse_request(const string &request, HttpRequest* out) {
  HttpRequest req("/");  // by default, get "/". //setting parameter
  
  string requestCopy(request);

  if (request.length() == 0) {
    return false;
  }

  boost::algorithm::trim(requestCopy);
  vector<string> reqComponents;
  boost::split(reqComponents, requestCopy, boost::is_any_of("\r\n"), boost::token_compress_on);
  
  std::vector<string> URLComponents;
  boost::split(URLComponents, reqComponents[0], boost::is_any_of(" "), boost::token_compress_on);
  req.set_uri(URLComponents[1]);
  std::vector<string> headerComponents;
  
  for(uint16_t i = 1; i < reqComponents.size(); i++) {
    string currLine = reqComponents[i];
    boost::to_lower(currLine);
    uint16_t len = currLine.length();
    size_t index = currLine.find(":");
    if (index == std::string::npos) {
      continue;
    }
    if(currLine == "\r\n") {
      break;
    }
    string key = currLine.substr(0, index);
    string val = currLine.substr(index+1, (len - 4));
    boost::algorithm::trim(key);
    boost::algorithm::trim(val);
    req.AddHeader(key, val);
  }
  *out = req;
  return true;
}

}  // namespace searchserver
