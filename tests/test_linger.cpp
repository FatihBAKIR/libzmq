/*
    Copyright (c) 2007-2013 Contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../include/zmq_utils.h"
#include <stdio.h>
#include <string.h>
#include "testutil.hpp"

#define URL "tcp://127.0.0.1:5560"

// sender connects a PUSH socket
// and sends a message of a given size,
// closing the socket and terminating the context immediately.
// LINGER should prevent this from dropping messages.

static void sender(void *vsize)
{
    void *context = zmq_ctx_new();
    void *push = zmq_socket(context, ZMQ_PUSH);
    zmq_msg_t msg;
    size_t size = ((size_t *) vsize)[0];

    int rc = zmq_connect(push, URL);
    assert (rc == 0);
  
    rc = zmq_msg_init_size(&msg, size);
    assert (rc == 0);

    char *buf = (char *) zmq_msg_data(&msg);
    assert (buf != NULL);
    for(size_t i = 0; i < size; ++i) {
        buf[i] = 'x';
    }

    printf("Sending %lu B ... ", zmq_msg_size(&msg));

    rc = zmq_msg_send(&msg, push, 0);
    assert (rc == size);

    rc = zmq_msg_close(&msg);
    assert (rc == 0);

    rc = zmq_close(push);
    assert (rc == 0);

    rc = zmq_ctx_term(context);
    assert (rc == 0);
}

int main (void)
{
    void *context = zmq_ctx_new();
    void *pull = zmq_socket(context, ZMQ_PULL);
    zmq_msg_t msg;
    
    int rc = zmq_bind(pull, URL);
    assert (rc == 0);
    
    // Symptom of test failure is message never received
    
    int rcvtimeo = 1000;
    rc = zmq_setsockopt(pull, ZMQ_RCVTIMEO, &rcvtimeo, sizeof (int));
    assert (rc == 0);
    
    for(int p = 0; p < 25; p += 4)
    {
      size_t size = 1 << p;
      
      // Start the sender thread
      void *send_thread = zmq_threadstart(&sender, (void *) &size);
      
      zmq_msg_init(&msg);
      rc = zmq_msg_recv(&msg, pull, 0);
      if (rc == -1 ) {
        printf("%s\n", zmq_strerror(zmq_errno()));
      } else {
        printf("Received.\n");
      }
      assert (rc == size);
      zmq_msg_close(&msg);
      zmq_threadclose(send_thread);
    }
    zmq_close(pull);
    zmq_ctx_term(context);
  
  return 0;
}