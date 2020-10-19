#ifndef __EXAMPLE_COMMON__
#define __EXAMPLE_COMMON__

#define XPUB_ENDPOINT "tcp://localhost:9200"
#define XSUB_ENDPOINT "tcp://localhost:9210"

const int TOPIC_LENGTH = 3;

std::string RECEIVE_TOPIC  = std::string("\x08\x10\x01", TOPIC_LENGTH);
std::string RESPONSE_TOPIC = std::string("\x08\x10\x02", TOPIC_LENGTH);
std::string WELCOME_TOPIC = std::string("\xF3\x00\x00", TOPIC_LENGTH);

#endif
