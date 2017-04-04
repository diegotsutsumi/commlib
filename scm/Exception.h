// SocketException class

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include <string>

class Exception
{
 public:
  Exception ( std::string s ) : m_s ( s ) {};
  ~Exception (){};

  std::string description() { return m_s; }

 protected:

  std::string m_s;

};

class SocketException : public Exception
{
public:
	SocketException(std::string s) : Exception(s){};
	~SocketException(){};
};
class ChannelException : public Exception
{
public:
	ChannelException(std::string s) : Exception(s){};
	~ChannelException(){};
};

#endif
