#include <iostream>
#include <boost/asio.hpp>

using namespace std;
using namespace boost::asio;

string h[5];
string p[5];
string f[5];

io_service ioservice;

void parsearg()
{
    string arg = string(getenv("QUERY_STRING"));
    for(int i=0;i<5;i++)
    {
        h[i] = arg.substr(arg.find("h"+to_string(i)));
        h[i] = h[i].substr(3,h[i].find("&")-3);
        p[i] = arg.substr(arg.find("p"+to_string(i)));
        p[i] = p[i].substr(3,p[i].find("&")-3);
        f[i] = arg.substr(arg.find("f"+to_string(i)));
        f[i] = f[i].substr(3,f[i].find("&")-3);
    }
}

void print_html_template()
{
    string heads = "";
    string tds = "";
    for(int i=0;i<5;i++)
    {
        if(h[i]=="")break;
        heads += "<th scope=\"col\">"+h[i]+":"+p[i]+"</th>\n";
        tds += "<td><pre id=\"s"+to_string(i)+"\" class=\"mb-0\"></pre></td>\n";
    }
    printf(R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>NP Project 3 Sample Console</title>
    <link
      rel="stylesheet"
      href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
      integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
      crossorigin="anonymous"
    />
    <link
      href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
      rel="stylesheet"
    />
    <link
      rel="icon"
      type="image/png"
      href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
    />
    <style>
      * {
        font-family: 'Source Code Pro', monospace;
        font-size: 1rem !important;
      }
      body {
        background-color: #212529;
      }
      pre {
        color: #cccccc;
      }
      b {
        color: #01b468;
      }
    </style>
  </head>
  <body>
    <table class="table table-dark table-bordered">
      <thead>
        <tr>
          %s
        </tr>
      </thead>
      <tbody>
        <tr>
          %s
        </tr>
      </tbody>
    </table>
  </body>
</html>
)",heads.c_str(),tds.c_str());
}

void escape(std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '\n':  buffer.append("&NewLine;");       break;
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(&data[pos], 1); break;
        }
    }
    data.swap(buffer);
}

void output_shell(int session,std::string content)
{
    escape(content);
    printf("<script>document.getElementById('s%d').innerHTML += '%s';</script>",session,content.c_str());
    fflush(stdout);
}
void output_command(int session,std::string content){
    escape(content);
    printf("<script>document.getElementById('s%d').innerHTML += '<b>%s</b>';</script>",session,content.c_str());
    fflush(stdout);
}

class session
  : public std::enable_shared_from_this<session>
{
public:
    session(std::string h,std::string p,std::string f,int i)
      : h(h),p(p),f(f),s_number(i)
    {
    }

    void start()
    {
        auto self(shared_from_this());
        ip::tcp::resolver::query q{h, p};
        _resolv.async_resolve(q, 
        [this, self](const boost::system::error_code &ec,ip::tcp::resolver::iterator it)
        {
            if (!ec)do_connect(it);
        }
        );
    }

private:
    void do_connect(ip::tcp::resolver::iterator it)
    {
        auto self(shared_from_this());
        _socket.async_connect(*it, 
        [this, self](const boost::system::error_code &ec)
        {
            if (!ec)
            {
                do_read();
            }
        });
    }

    void do_read()
    {
        auto self(shared_from_this());
        boost::asio::async_read_until(_socket,sockiobuf,"% ",
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
        {
            if (!ec)
            {
                char * buffer = new char [bytes_transferred+1];
                iostream(&sockiobuf).read(buffer,bytes_transferred);
                buffer[bytes_transferred] = '\x00';
                output_shell(s_number,std::string(buffer));
                do_write();
            }
        });
    }

    void do_write()
    {
        
    }

    boost::asio::streambuf sockiobuf;
    std::string h;
    std::string p;
    std::string f;
    int s_number;
    ip::tcp::resolver _resolv{ioservice};
    ip::tcp::socket _socket{ioservice};
};

int main(){
    printf("Content-type: text/html\r\n\r\n");
    
    parsearg();
    print_html_template();

    for(int i=0;i<5;i++)
    {
        if(h[i]=="")break;
        std::make_shared<session>(h[i],p[i],f[i],i)->start();
    }

    ioservice.run();
    return 0;
}