# example_zmq
C++ ZMQ example using PUB/SUB via a ZMQ XPUB/XSUB proxy. See
<URL> about this.

## Overview

There are 3 programs `zmq_proxy`, `zmq_producer` and `zmq_subscriber`.

The `zmq_proxy` creates the XPUB/XSUB proxy for the messaging, it will
create the appropriate ports and sends a welcome message when clients
connect to the XPUB socket.

The `zmq_subscriber` subscribes to messages with the XPUB endpoint
on the topic `interesting_topic`. When it receives a message it will
really all the parts of the message, both topic and the main message
part. It will then ACK the message on a separate topic `response_topic`.

The `zmq_producer` sends a message every 5 seconds for a total of 20
messages. It sends them to the the `interesting_topic` that the
`zmq_subscriber` would be listening to. It also listens to the
`response_topic` to listen for ACK messages (it doesn't do anything if
a message isn't ACKed).

## Compiling

### zmq_proxy
```
$ g++ zmq_proxy.cpp -lzmq -o zmq_proxy
```

### zmq_producer
```
$ g++ zmq_producer.cpp -lzmq -pthread -o zmq_producer
```

### zmq_subscriber
```
$ g++ zmq_subscriber.cpp -lzmq -pthread -o zmq_subscriber
```

## Running

Start the proxy with:
```
$ ./zmq_proxy &
```

Then start the subscriber:
```
$ ./zmq_subscriber &
```

Finally start the producer:
```
$ ./zmq_producer
```

