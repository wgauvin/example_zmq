#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <thread>
#include <zmq.hpp>
#include "common.h"

using namespace std;

int main(int argc, char *argv[])
{
    // "You should create and use exactly one context in your process."
    zmq::context_t context(2);

    // Init XSUB socket
    zmq::socket_t xsub_socket(context, ZMQ_XSUB);
    std::string xsub_endpoint("tcp://*:9210");
    try
    {
        // The port number here is the XSUB port of the Msg Proxy service (9200)
        xsub_socket.bind(xsub_endpoint);
    }
    catch (zmq::error_t e)
    {
        cerr << "Error connection to " << xsub_endpoint << ". Error is: " << e.what() << endl;
        exit(1);
    }

    // Init XPUB socket
    zmq::socket_t xpub_socket(context, ZMQ_XPUB);
    std::string xpub_endpoint("tcp://*:9200");
    try
    {
        // The port number here is the XSUB port of the Msg Proxy service (9200)
        xpub_socket.bind(xpub_endpoint);
        xpub_socket.setsockopt(ZMQ_XPUB_VERBOSER, 1);
        xpub_socket.setsockopt(ZMQ_XPUB_WELCOME_MSG, WELCOME_MESSAGE.c_str());
    }
    catch (zmq::error_t e)
    {
        cerr << "Error connection to " << xpub_endpoint << ". Error is: " << e.what() << endl;
        exit(1);
    }

    // need to create the proxy

    zmq::proxy(xsub_socket, xpub_socket);

    return 0;
}
