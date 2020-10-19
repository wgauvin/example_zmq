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

std::string RECEIVE_TOPIC = "interesting_topic";
std::string RESPONSE_TOPIC = "response_topic";

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
    subscriber.setsockopt(ZMQ_SUBSCRIBE, WELCOME_MESSAGE.c_str(), WELCOME_MESSAGE.length());
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
    // only need to do this once
    // zmq::message_t topic_msg(RESPONSE_TOPIC.length());
    // memcpy(topic_msg.data(), RESPONSE_TOPIC.c_str(), RESPONSE_TOPIC.length());

    size_t int_size = sizeof(int);

    while (true)
    {

      zmq::poll(p.data(), 1, -1);
      if (p[0].revents & ZMQ_POLLIN)
      {

        int recvMore = 1;
        while (recvMore)
        {
          zmq::message_t msg;
          subscriber.recv(msg, zmq::recv_flags::none);
          std::string msg_txt;
          msg_txt.assign(static_cast<char *>(msg.data()), msg.size());
          cout << "[SUBSCRIBER]: " << msg_txt << endl;

          subscriber.getsockopt(ZMQ_RCVMORE, &recvMore, &int_size);

          if (!recvMore)
          {
            // This isn't working! 
            if (msg_txt == "WELCOME")
            {
              cout << "[SUBSCRIBER]: Welcome message recved. Okay to do stuff";
              continue;
            }
            zmq::message_t topic_msg(RESPONSE_TOPIC.length());
            memcpy(topic_msg.data(), RESPONSE_TOPIC.c_str(), RESPONSE_TOPIC.length());

            zmq::message_t ack_msg(3);
            std::string msg_text = "ACK";
            memcpy(ack_msg.data(), msg_text.c_str(), msg_text.length());

            publisher.send(topic_msg, zmq::send_flags::sndmore);
            publisher.send(ack_msg, zmq::send_flags::none);

            cout << "[SUBSCRIBER]: Sent " << msg_text << " to " << RESPONSE_TOPIC << endl;
          }
        }
      }
    }
  });

  subs_thread.join();
  return 0;
}
