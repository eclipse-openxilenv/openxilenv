/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define _GNU_SOURCE
#include <stdint.h>
#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include<signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sched.h>


#include "ThrowError.h"
#include "MemoryAllocation.h"

#include "CpuClock.h"
#include "RealtimeScheduler.h"
#include "Fifos.h"

#include "StructRM.h"
#include "RemoteMasterReqToClient.h"
#include "CanFifo.h"
#include "RemoteMasterDecoder.h"

#include "RemoteMasterServer.h"

//#define LOG_REMOTE_MASTER_CALL     "/tmp/remote_master_calls"

#define UNUSED(x) (void)(x)

// same in RemoteMasterDecoder.c!!!
#define BUFFER_SIZE (2 * 1024 * 1024)

/*
static uint64_t my_rdtsc(void) {
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d << 32) | a;
}*/

static int RetVal;

static int main_accept_socket;

/*
* This will handle connection for each client
* */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    char *receive_buffer;  
    char *transmit_buffer;  
    int buffer_pos, buffer_pos2;
	int tx_buffer_pos;
    uint32_t receive_message_size;
    int32_t transmit_message_size;
    int len;
    //RM_PACKAGE_HEADER *only_for_debug;
	cpu_set_t mask;
	pthread_t thread;
	struct sched_param schedp;

#ifdef LOG_REMOTE_MASTER_CALL
	FILE *fh;
	char log_name[100];
	sprintf(log_name, "%s_%i.txt", LOG_REMOTE_MASTER_CALL, sock);
	fh = fopen(log_name, "wt");
#endif

    receive_buffer = /*not my_*/ malloc(BUFFER_SIZE);
    transmit_buffer = /*not my_*/ malloc(BUFFER_SIZE);

	CPU_ZERO(&mask);
	CPU_SET(1, &mask);  // CPU1
	thread = pthread_self();
	if (pthread_setaffinity_np(thread, sizeof(mask), &mask) != 0) {
		printf("Could not set CPU affinity to CPU %d\n", 1);
	}
	memset(&schedp, 0, sizeof(schedp));
	schedp.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &schedp)) {
        printf("failed to set priority to %d\n", 99);
    }

	printf("connection_handler() running on cpu %i\n", sched_getcpu());
	while (1) {
		buffer_pos = 0;
		receive_message_size = 0;
		do {
            len = recv(sock, receive_buffer + buffer_pos, BUFFER_SIZE - buffer_pos, MSG_NOSIGNAL); // 0);
			if (len > 0) {
				buffer_pos += len;
				if (receive_message_size == 0) {
					receive_message_size = *(uint32_t*)receive_buffer;
				}
			} else {
                ThrowError(1, "Failed receiving\n");
			}
        } while (buffer_pos < (int)receive_message_size);

        if (receive_message_size == 0) {
            printf("connection closed\n");
            goto __OUT;
        }
		if (sched_getcpu() != 1) {
			printf("Connection handler should be running on CPU1\n");
		}

		buffer_pos2 = 0; 
        do {
			transmit_message_size = DecodeCommand((RM_PACKAGE_HEADER*)(receive_buffer + buffer_pos2), (RM_PACKAGE_HEADER*)transmit_buffer);

#ifdef LOG_REMOTE_MASTER_CALL
			if (fh != NULL) {
				fprintf(fh, "%llu (%i/%i) %i: %i %i %i %i\n", my_rdtsc(), ((RM_PACKAGE_HEADER*)receive_buffer)->ChannelNumber, ((RM_PACKAGE_HEADER*)receive_buffer)->PackageCounter, ((RM_PACKAGE_HEADER*)receive_buffer)->Command, ((RM_PACKAGE_HEADER*)receive_buffer)->SizeOf, receive_message_size, transmit_message_size, len);
				fflush(fh);
			}
#endif

			if (transmit_message_size > 0) {
				tx_buffer_pos = 0;
				*(uint32_t*)transmit_buffer = transmit_message_size;
				do {
					len = send(sock, transmit_buffer + tx_buffer_pos, transmit_message_size - tx_buffer_pos, 0);
					if (len > 0) {
						tx_buffer_pos += len;
					} else {
                        ThrowError(1, "Failed to transmit\n");
					}
				} while (tx_buffer_pos < transmit_message_size);
			} else if (transmit_message_size < 0) {
				// transmit_message_size negativ dann Beenden!
                if (transmit_message_size == -1) {
                    printf("\nswill be closed\n");
                    shutdown(main_accept_socket, SHUT_RDWR);
                } else if (transmit_message_size == -2) {
                    printf("\nthread will be closed\n");
                    //shutdown(sock, SHUT_RDWR);
                    close(sock);
                }
                goto __OUT;  // terminate thread
			}
            buffer_pos2 += receive_message_size;   // Switch to the next message
            if (buffer_pos2 + (int)sizeof(receive_message_size) >= (buffer_pos)) break;
			receive_message_size = *(uint32_t*)(receive_buffer + buffer_pos2); 
        } while ((buffer_pos2 + (int)receive_message_size) <= buffer_pos);
		if (buffer_pos2 != buffer_pos) {
			printf("there is something left in the buffer\n");
		}
    }
 __OUT:
    free(receive_buffer);
    free(transmit_buffer);
    pthread_exit(&RetVal);
    return NULL;
}

int inner_main(int argc , char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
    int first_connection = 1;
	pthread_attr_t attr;
	int ret;

    CalibrateCpuTicks();

    signal(SIGPIPE, SIG_IGN);

	InitCANFifoCriticalSection();

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Could not create socket\n");
    }
    puts("Socket created");
     
    {
        int optval = 1;
        setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        printf("bind failed. Error\n");
        return 1;
    }
    puts("bind done");

    main_accept_socket = socket_desc;
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    printf("Waiting for incoming connections...\n");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) > 0 ) {
        printf("Connection accepted\n");
        if (first_connection) {
            set_client_socket_for_request(client_sock);
            first_connection = 0;
            printf("first socket are used for events to client\n");
        } else {

            /* Initialize pthread attributes (default values) */
            ret = pthread_attr_init(&attr);
            if (ret) {
                printf("init pthread attributes failed\n");
                return -1;
            }

            /* Set a specific stack size  */
            ret = pthread_attr_setstacksize(&attr, 256 * 1024);
            if (ret) {
                printf("pthread setstacksize failed\n");
                return -1;
            }

            if (pthread_create(&thread_id, NULL, connection_handler, (void*)&client_sock) < 0) {
                printf("could not create thread\n");
                return 1;
            }

			pthread_setname_np(thread_id, "connection_handler");

            printf("Handler assigned\n");
        }
    }
     
    if (client_sock < 0) {
        printf("accept failed\n");
        return 1;
    }
     
    return 0;
}
