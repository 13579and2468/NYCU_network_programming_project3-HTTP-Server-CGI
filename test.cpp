#include <iostream>
#include <unistd.h>

using namespace std;

int main(){
    cout<<"Content-type: text/html\r\n\r\n";
    fflush(stdout);
    cout<<"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    fflush(stdout);
    sleep(3);
    cout<<"bbbbbbbbbbbbbbbbbbbbbbbb</body></body>"<<endl;
    fflush(stdout);
}