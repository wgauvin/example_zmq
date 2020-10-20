#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <thread>
#include <zmq.hpp>

#include "common.h"

using namespace std;

/*
    This code has adapted from https://ogbe.net/blog/zmq_helloworld.html

    However, this connects to a XPUB/XSUB proxy.  A simple Python script
    connects at the other end but it is still a simple hello world to
    show how to used this in C++ (easier for the threading).
*/

int main(int argc, char *argv[])
{
  // "You should create and use exactly one context in your process."
  zmq::context_t context(2);

  // the main thread runs the publisher and sends messages periodically
  zmq::socket_t publisher(context, ZMQ_PUB);
  std::string pub_transport(XSUB_ENDPOINT);
  try
  {
    // The port number here is the XSUB port of the Msg Proxy service (9200)
    publisher.connect(pub_transport);
  }
  catch (zmq::error_t e)
  {
    cerr << "Error connection to " << pub_transport << ". Error is: " << e.what() << endl;
    exit(1);
  }

  // in a seperate thread, poll the socket until a message is ready. when a
  // message is ready, receive it, and print it out. then, start over.
  zmq::socket_t subscriber(context, ZMQ_SUB);
  std::string sub_transport(XPUB_ENDPOINT);
  try
  {
    // The subscriber socket
    // The port number here is the XSUB port of the Msg Proxy service (9210)
    subscriber.setsockopt(ZMQ_SUBSCRIBE, WELCOME_TOPIC.c_str(), WELCOME_TOPIC.length());
    subscriber.setsockopt(ZMQ_SUBSCRIBE, RECEIVE_TOPIC.c_str(), RECEIVE_TOPIC.length());
    subscriber.connect(sub_transport);

    // helps with slow connectors!
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  catch (zmq::error_t e)
  {
    cerr << "Error connection to " << sub_transport << ". Error is: " << e.what() << endl;
    exit(1);
  }

  // to use zmq_poll correctly, we construct this vector of pollitems
  std::vector<zmq::pollitem_t> p = {{subscriber, 0, ZMQ_POLLIN, 0}};

  // the subscriber thread that returns the same message back to the publisher.
  std::thread subs_thread([&subscriber, &p, &publisher]() {
    size_t int_size = sizeof(int);

    while (true)
    {
      zmq::poll(p.data(), 1, -1);
      if (p[0].revents & ZMQ_POLLIN)
      {
        multipart_msg_t msg;

        recv_multipart_msg(&subscriber, &msg);

        if (msg.topic == WELCOME_TOPIC) {
          cout << "[SUBSCRIBER]: Welcome message recved. Okay to do stuff" << endl;
          continue;
        }

        for (auto m : msg.msgs) {
          cout << "[SUBSCRIBER]: Received '" << m << "' on request topic" << endl;
        }

        multipart_msg_t ack_msg;
        ack_msg.topic = RESPONSE_TOPIC;
        ack_msg.msgs.push_back("ACK"s);

        send_multipart_msg(&publisher, &ack_msg);

        cout << "[SUBSCRIBER]: Sent ACK to response topic" << endl;
      }
    }
  });

  subs_thread.join();
  return 0;
}
