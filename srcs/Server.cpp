#include<iostream>
#include"../includes/Server.hpp"
using namespace std;

Server::Server()
{
    cout << "Server set up" << endl;
}

Server::~Server()
{
    cout << "Server shut down" << endl;
}

void Server::run_server()
{
    cout << "Server activited" << endl;
}