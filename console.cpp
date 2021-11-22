#include <iostream>

using namespace std;

string h[5];
string ps[5];
int p[5];
string f[5];
void parsearg()
{
    string arg = string(getenv("QUERY_STRING"));
    for(int i=0;i<5;i++)
    {
        h[i] = arg.substr(arg.find("h"+to_string(i)));
        h[i] = h[i].substr(0,h[i].find("&"));
        ps[i] = arg.substr(arg.find("p"+to_string(i)));
        ps[i] = ps[i].substr(0,ps[i].find("&"));
        p[i] = stoi(ps[i]);
        f[i] = arg.substr(arg.find("f"+to_string(i)));
        f[i] = f[i].substr(0,f[i].find("&"));
    }
}
int main(){
    parsearg();
    return 0;
}