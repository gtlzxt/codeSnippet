#include <iostream>
#include <string.h>
#include "client.h"
#include "server.h"
using namespace std;

int main(int argc, char** argv) {
    if(argc < 2)
    {
        cout << "arguments invalid" << endl;
        return 0;
    }
    if(strcmp(argv[1], "client") == 0 && argc < 3)
    {
        cout <<"need to set server ip" << endl;
        return 0;
    }

    if(strcmp(argv[1], "server") == 0)
    {
        cout << "server start" << endl;
        server();
    }
    else if(strcmp(argv[1], "client") == 0)
    {
        cout << "client start" << endl;
        client(argv[2]);
    }
    else
    {
        cout << "invalid arguments" << endl;
    }
    
}
