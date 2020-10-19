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
        // first part of msg is the topic
        zmq::message_t topic_msg(TOPIC_LENGTH);
        subscriber.recv(topic_msg, zmq::recv_flags::none);
        std::string topic_msg_txt;
        topic_msg_txt.assign(static_cast<char *>(topic_msg.data()), topic_msg.size());

        if (topic_msg_txt == WELCOME_TOPIC) {
          cout << "[SUBSCRIBER]: Welcome message recved. Okay to do stuff" << endl;

          int recvMore;
          subscriber.getsockopt(ZMQ_RCVMORE, &recvMore, &int_size);
          if (recvMore) {
            cout << "[SUBSCRIBER]: Welcome message has more though!" << endl;
          }

          continue;
        }

        int recvMore;
        subscriber.getsockopt(ZMQ_RCVMORE, &recvMore, &int_size);
        while (recvMore)
        {
          zmq::message_t msg;
          subscriber.recv(msg, zmq::recv_flags::none);
          std::string msg_txt;
          msg_txt.assign(static_cast<char *>(msg.data()), msg.size());
          cout << "[SUBSCRIBER]: Received '" << msg_txt << "' on request topic" << endl;

          subscriber.getsockopt(ZMQ_RCVMORE, &recvMore, &int_size);
          if (!recvMore)
          {
            zmq::message_t response_topic_msg(RESPONSE_TOPIC.length());
            memcpy(response_topic_msg.data(), RESPONSE_TOPIC.c_str(), RESPONSE_TOPIC.length());

            zmq::message_t ack_msg(3);
            std::string ack_msg_text = "ACK";
            memcpy(ack_msg.data(), ack_msg_text.c_str(), ack_msg_text.length());

            publisher.send(response_topic_msg, zmq::send_flags::sndmore);
            publisher.send(ack_msg, zmq::send_flags::none);

            cout << "[SUBSCRIBER]: Sent ACK to response topic" << endl;
          }
        }
      }
    }
  });

  subs_thread.join();
  return 0;
}
