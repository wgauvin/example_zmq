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

std::string SEND_TOPIC = "interesting_topic";
std::string RESPONSE_TOPIC = "response_topic";

int main(int argc, char *argv[])
{
  // "You should create and use exactly one context in your process."
  zmq::context_t context(2);

  std::string pub_transport(XSUB_ENDPOINT);
  // the main thread runs the publisher and sends messages periodically
  zmq::socket_t publisher(context, ZMQ_PUB);
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

  std::string sub_transport(XPUB_ENDPOINT);
  // in a seperate thread, poll the socket until a message is ready. when a
  // message is ready, receive it, and print it out. then, start over.
  //
  // The subscriber socket
  // The port number here is the XSUB port of the Msg Proxy service (9210)
  zmq::socket_t subscriber(context, ZMQ_SUB);
  try
  {
    subscriber.setsockopt(ZMQ_SUBSCRIBE, WELCOME_MESSAGE.c_str(), WELCOME_MESSAGE.length());
    subscriber.connect(sub_transport);
    subscriber.setsockopt(ZMQ_SUBSCRIBE, RESPONSE_TOPIC.c_str(), RESPONSE_TOPIC.length());

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
  std::thread subs_thread([&subscriber]() {
    size_t int_size = sizeof(int);

    while (true)
    {
      int recvMore = 1;
      while (recvMore)
      {
        zmq::message_t msg;
        subscriber.recv(msg, zmq::recv_flags::none);
        std::string msg_txt;
        msg_txt.assign(static_cast<char *>(msg.data()), msg.size());
        cout << "[PUBLISHER]: Received " << msg_txt << endl;

        subscriber.getsockopt(ZMQ_RCVMORE, &recvMore, &int_size);
      }
    }
  });

  for (auto i = 0; i < 20; i++)
  {
    zmq::message_t topic_msg(SEND_TOPIC.length());
    memcpy(topic_msg.data(), SEND_TOPIC.c_str(), SEND_TOPIC.length());

    std::string msg_text = "Hello World!";
    zmq::message_t msg(msg_text.length());
    memcpy(msg.data(), msg_text.c_str(), msg_text.length());

    // need to send topic msg first, then actual message
    publisher.send(topic_msg, zmq::send_flags::sndmore);
    publisher.send(msg, zmq::send_flags::none);

    cout << "[PUBLISHER]: Sent " << msg_text << " to " << SEND_TOPIC << endl;

    // add some delay
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  subs_thread.join();
  return 0;
}
