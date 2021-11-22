#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <sstream>

using boost::asio::ip::tcp;
namespace bp = boost::process;
using bp::posix::fd;
using namespace std;

int fork_unitil_success(){
  int r;
  while( (r=fork())==-1 );
  return r;
}

// split line into tokens
vector<string> split(string s){
  stringstream ss(s);
  string token;
  vector<string> v;
  while(ss>>token){
    v.push_back(token);
  }
  return v;
}

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_,sockiobuf,"\r\n\r\n",
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
        {
          if (!ec)
          {
            do_write(bytes_transferred);
          }
        });
  }

  void mysetenv(iostream& ios)
  {
    string line;
    getline(ios,line);
    auto tokens = split(line.substr(0,line.length()-1));
    setenv("REQUEST_METHOD",tokens[0].c_str(),1);
    setenv("REQUEST_URI",tokens[1].c_str(),1);
    setenv("QUERY_STRING",tokens[1].substr(tokens[1].find("?")+1).c_str(),1);
    setenv("SERVER_PROTOCOL",tokens[2].c_str(),1);
    
    while(getline(ios,line))
    {
        if(line=="\r")break;
        auto tokens = split(line.substr(0,line.length()-1));
        if(tokens[0]=="Host:")setenv("HTTP_HOST",tokens[1].c_str(),1);
    }

    setenv("REMOTE_ADDR",socket_.remote_endpoint().address().to_string().c_str(),1);
    setenv("REMOTE_PORT",to_string(socket_.remote_endpoint().port()).c_str(),1);
    setenv("SERVER_ADDR",socket_.local_endpoint().address().to_string().c_str(),1);
    setenv("SERVER_PORT",to_string(socket_.local_endpoint().port()).c_str(),1);
  }

  void do_write(std::size_t bytes_transferred)
  {
    int pid = fork_unitil_success();
    if(pid==0)
    {
        auto self(shared_from_this());
        iostream ios(&sockiobuf);
        mysetenv(ios);
        boost::asio::async_write(socket_, boost::asio::buffer("HTTP/1.1 200 OK\r\n"),
            [this, self](boost::system::error_code ec, std::size_t /*bytes_transferred*/)
            {
                if (!ec)
                {
                    string target = "."+string(getenv("REQUEST_URI"));
                    target = target.substr(0,target.find("?"));
                    bp::spawn(target.c_str(), fd.bind(0, socket_.native_handle()), fd.bind(1, socket_.native_handle()));
                    exit(0);
                }
            });
    }else
    {
    }
  }

  tcp::socket socket_;
  boost::asio::streambuf sockiobuf;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
    //setting SIGCHILD tp ignore will give zombie to init process
  signal(SIGCHLD,SIG_IGN);
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: http_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}