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


#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <getopt.h>
#include <pwd.h>


#define BUFFER_SIZE (1024 * 1024)
#define PROTOCOL_VERSION  1
#define COMMAND_PING                   0
#define COMMAND_IS_RUNNING     1
#define COMMAND_COPY_FILE_START        2
#define COMMAND_COPY_FILE_NEXT         3
#define COMMAND_COPY_FILE_END          4
#define COMMAND_EXECUTE_COMMAND        5
#define COMMAND_EXECUTE_FILE           6
#define COMMAND_KILL_PROCESS           7
#define COMMAND_CLOSE_CONNECTION       8

#define COMMAND_GET_PARTITION_BLOCK_SIZE       101
#define COMMAND_GET_PARTITION_SIZE             102
#define COMMAND_READ_PARTITION_START           103
#define COMMAND_READ_PARTITION_NEXT            104
#define COMMAND_READ_PARTITION_END             105

typedef struct {
	uint32_t Size;
	uint32_t Version;
	uint32_t Command;
	uint32_t Counter;
	uint32_t DataSize;
	uint32_t Filler;
	char Data[8];           // Kann laenger sein
} PACKAGE_HEADER_REQ;

typedef struct {
	uint32_t Size;
	uint32_t Version;
	uint32_t Command;
	uint32_t Counter;
	uint32_t Ret;
	uint32_t Filler;
	char ErrorString[8];    // Kann laenger sein
} PACKAGE_HEADER_ACK;


int CommandPing(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
	transmit_buffer->Command = COMMAND_PING;
	transmit_buffer->Counter = receive_buffer->Counter;
	transmit_buffer->Ret = 0;
	transmit_buffer->ErrorString[0] = 0;
	return sizeof(PACKAGE_HEADER_ACK);
}

char Filename[1024];
int FileHandle;
uint32_t BlockCounter;

char *ReplaceTildeCharWithHomeDirectory(char *Src)
{
    char *p;
    int SrcLen;
    int TildeCounter = 0;
    char *Ret;

    p = Src;
    while (*p != 0) {
        if (*p == '~') TildeCounter++;
        p++;
    }
    SrcLen = p - Src;

    if (TildeCounter) {
        char *homedir;
        int NeedLen;
        int HomeDirLen;
        if ((homedir = getenv("HOME")) == NULL) {
            homedir = getpwuid(getuid())->pw_dir;
        }
        HomeDirLen = strlen (homedir);
        NeedLen = SrcLen + TildeCounter * HomeDirLen + 1;
        Ret = (char*)malloc(NeedLen);
        if (Ret == NULL) {
            return NULL;
        } else {
            char *s = Src;
            char *d = Ret;
            while (*s != 0) {
                if (*s == '~') {
                    memcpy(d, homedir, HomeDirLen);
                    d += HomeDirLen;
                } else {
                   *d++ = *s;
                }
                s++;
            }
            *d = 0;
        }
        return Ret;
    } else {
        Ret = (char*)malloc(SrcLen + 1);
        if (Ret != NULL) memcpy(Ret, Src, SrcLen + 1);
        return Ret;
    }
}

int CommandCopyFileStart(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    char buf[4096]; /* PATH_MAX incudes the \0 so +1 is not required */
    char *res;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    BlockCounter = 0;
    transmit_buffer->Command = COMMAND_COPY_FILE_START;
    transmit_buffer->Counter = receive_buffer->Counter;

    // ~ ersetzen
    if (receive_buffer->Data[0] == '~') {
        char *homedir;
        if ((homedir = getenv("HOME")) == NULL) {
            homedir = getpwuid(getuid())->pw_dir;
        }
        strcpy(buf, homedir);
        strcat(buf, &(receive_buffer->Data[1]));
        res = buf;
    } else {
        res = receive_buffer->Data;
    }

    //res = realpath(receive_buffer->Data, buf);
    //if (res == NULL) res = receive_buffer->Data;
    FileHandle = open(res, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (FileHandle > 0) {
        transmit_buffer->Ret = 0;
        transmit_buffer->ErrorString[0] = 0;
        memset(Filename, 0, sizeof(Filename));
        strncpy(Filename, receive_buffer->Data, sizeof(Filename) - 1);
    } else {
        transmit_buffer->Ret = -1;
        transmit_buffer->ErrorString[0] = 0;
        sprintf(transmit_buffer->ErrorString, "cannot open file %s", res);
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}

int CommandCopyFileNext(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    transmit_buffer->Command = COMMAND_COPY_FILE_NEXT;
    transmit_buffer->Counter = receive_buffer->Counter;
    if (FileHandle <= 0) {
        transmit_buffer->Ret = -1;
        sprintf(transmit_buffer->ErrorString, "file ist not opened command \"COMMAND_COPY_FILE_START\" is missing");
    } else if (BlockCounter != receive_buffer->Counter) {
        transmit_buffer->Ret = -1;
        sprintf(transmit_buffer->ErrorString, "wrong block number %i expected %i inside command \"COMMAND_COPY_FILE_NEXT\"", receive_buffer->Counter, BlockCounter);
    } else {
        if (write(FileHandle, receive_buffer->Data, receive_buffer->DataSize) == receive_buffer->DataSize) {
            BlockCounter++;
            transmit_buffer->Ret = 0;
            transmit_buffer->ErrorString[0] = 0;
        } else {
            transmit_buffer->Ret = -1;
            sprintf(transmit_buffer->ErrorString, "cannot copy all data to file \"%s\" during command \"COMMAND_COPY_FILE_START\"", Filename);
        }
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}

int CommandCopyFileEnd(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    transmit_buffer->Command = COMMAND_COPY_FILE_END;
    transmit_buffer->Counter = receive_buffer->Counter;
    if (FileHandle <= 0) {
        transmit_buffer->Ret = -1;
        sprintf(transmit_buffer->ErrorString, "file ist not opened command \"COMMAND_COPY_FILE_START\" is missing");
    } else {
        close(FileHandle);
        FileHandle = 0;
        BlockCounter = 0;
        memset(Filename, 0, sizeof(Filename));
        transmit_buffer->Ret = 0;
        transmit_buffer->ErrorString[0] = 0;
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}

// Partition and file reads
int CommandGetPartionsBlockSize(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    int FileHandle;
    uint32_t BlockSize;

    transmit_buffer->Command = COMMAND_GET_PARTITION_BLOCK_SIZE;
    transmit_buffer->Counter = receive_buffer->Counter;

    printf("partition = \"%s\"\n", receive_buffer->Data);
    FileHandle = open(receive_buffer->Data, O_RDONLY, 0);

    if (FileHandle > 0) {
        ioctl(FileHandle, BLKSSZGET, &BlockSize);
        close(FileHandle);
        transmit_buffer->Ret = 0;
        *(uint32_t*)transmit_buffer->ErrorString = BlockSize;
    }
    else {
        transmit_buffer->Ret = -1;
        transmit_buffer->ErrorString[0] = 0;
        sprintf(transmit_buffer->ErrorString, "error");
    }
    return sizeof(PACKAGE_HEADER_ACK);
}

int CommandGetPartionsSize(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    int FileHandle;
    size_t Size;

    transmit_buffer->Command = COMMAND_GET_PARTITION_SIZE;
    transmit_buffer->Counter = receive_buffer->Counter;

    FileHandle = open(receive_buffer->Data, O_RDONLY, 0);

    if (FileHandle > 0) {
        ioctl(FileHandle, BLKGETSIZE64, &Size);
        close(FileHandle);
        transmit_buffer->Ret = 0;
        *(size_t*)transmit_buffer->ErrorString = Size;
    }
    else {
        transmit_buffer->Ret = -1;
        transmit_buffer->ErrorString[0] = 0;
        sprintf(transmit_buffer->ErrorString, "error");
    }
    return sizeof(PACKAGE_HEADER_ACK);
}


int CommandReadPartitionStart(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    BlockCounter = 0;
    transmit_buffer->Command = COMMAND_READ_PARTITION_START;
    transmit_buffer->Counter = receive_buffer->Counter;

    FileHandle = open(receive_buffer->Data, O_RDONLY, 0);
    if (FileHandle > 0) {
        transmit_buffer->Ret = 0;
        transmit_buffer->ErrorString[0] = 0;
        memset(Filename, 0, sizeof(Filename));
        strncpy(Filename, receive_buffer->Data, sizeof(Filename) - 1);
    }
    else {
        transmit_buffer->Ret = -1;
        transmit_buffer->ErrorString[0] = 0;
        sprintf(transmit_buffer->ErrorString, "cannot open file %s", receive_buffer->Data);
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}

int CommandReadPartitionNext(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    transmit_buffer->Command = COMMAND_READ_PARTITION_NEXT;
    transmit_buffer->Counter = receive_buffer->Counter;
    if (FileHandle <= 0) {
        transmit_buffer->Ret = -1;
        sprintf(transmit_buffer->ErrorString, "file ist not opened command \"COMMAND_READ_PARTITION_START\" is missing");
    }
    else if (BlockCounter != receive_buffer->Counter) {
        transmit_buffer->Ret = -1;
        sprintf(transmit_buffer->ErrorString, "wrong block number %u expected %u inside command \"COMMAND_READ_PARTITION_START\"", receive_buffer->Counter, BlockCounter);
    } else {
        if (receive_buffer->DataSize != sizeof(uint64_t)) {
            sprintf(transmit_buffer->ErrorString, "wrong data size %u expected %lu inside command \"COMMAND_READ_PARTITION_START\"", receive_buffer->DataSize, sizeof(uint64_t));
        }
        ssize_t BlockSize = (ssize_t)*(uint64_t*)receive_buffer->Data;
        if (read(FileHandle, transmit_buffer->ErrorString, BlockSize) == BlockSize) {
            BlockCounter++;
            transmit_buffer->Ret = 0;
            // give back data not error string !!!
            return sizeof(PACKAGE_HEADER_ACK) + BlockSize - sizeof(transmit_buffer->ErrorString);
        }
        else {
            transmit_buffer->Ret = -1;
            sprintf(transmit_buffer->ErrorString, "cannot copy all data from file \"%s\" during command \"COMMAND_READ_PARTITION_START\"", Filename);
        }
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}

int CommandReadPartitionEnd(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    transmit_buffer->Command = COMMAND_READ_PARTITION_END;
    transmit_buffer->Counter = receive_buffer->Counter;
    if (FileHandle <= 0) {
        transmit_buffer->Ret = -1;
        sprintf(transmit_buffer->ErrorString, "file ist not opened command \"COMMAND_READ_PARTITION_START\" is missing");
    }
    else {
        close(FileHandle);
        FileHandle = 0;
        BlockCounter = 0;
        memset(Filename, 0, sizeof(Filename));
        transmit_buffer->Ret = 0;
        transmit_buffer->ErrorString[0] = 0;
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}

int CommandExecuteCommand(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    char *WithoutTildeChar;
    transmit_buffer->Command = COMMAND_EXECUTE_COMMAND;
    transmit_buffer->Counter = receive_buffer->Counter;
    WithoutTildeChar = receive_buffer->Data;
    WithoutTildeChar = ReplaceTildeCharWithHomeDirectory(receive_buffer->Data);
    if (WithoutTildeChar != NULL) {
        transmit_buffer->Ret = system(WithoutTildeChar);
        free (WithoutTildeChar);
    }
    return sizeof(PACKAGE_HEADER_ACK);
}


char start_exec_name[1024];
int exec_state;

void *start_executable_connection_handler(void *file_name)
{
    char *DebugStr;
    exec_state = 1;
    DebugStr = file_name;
    system((char*)file_name);
    exec_state = 2;
    return NULL;
}

int CommandExecuteFile(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    pthread_t thread_id;
    int i;

    transmit_buffer->Command = COMMAND_EXECUTE_FILE;
    transmit_buffer->Counter = receive_buffer->Counter;
    exec_state = 0;
    memset(start_exec_name, 0, sizeof(start_exec_name));
    strncpy(start_exec_name, receive_buffer->Data, sizeof(start_exec_name) - 1);
    if (pthread_create(&thread_id, NULL, start_executable_connection_handler, (void*)start_exec_name) < 0) {
        sprintf(transmit_buffer->ErrorString, "could not create thread");
        return 1;
    }
    for (i = 0; i < 10; i++) {
        usleep(10000);   // 10ms warten
        if (exec_state) break;
    }
    if (exec_state == 1) {
        transmit_buffer->Ret = 0;
        transmit_buffer->ErrorString[0] = 0;
    } else {
        sprintf(transmit_buffer->ErrorString, "could start \"%s\"", start_exec_name);
        transmit_buffer->Ret = -1;
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}

int CommandIsRunning(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    transmit_buffer->Command = COMMAND_IS_RUNNING;
    transmit_buffer->Counter = receive_buffer->Counter;
    switch (exec_state) {
    case 1:
        transmit_buffer->Ret = 1;
        sprintf(transmit_buffer->ErrorString, "is running");
        break;
    case 0:
        transmit_buffer->Ret = 0;
        sprintf(transmit_buffer->ErrorString, "was not started");
        break;
    case 2:
        transmit_buffer->Ret = 0;
        sprintf(transmit_buffer->ErrorString, "was terminated");
        break;
    }
    return sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
}


int DecodeCommand(PACKAGE_HEADER_REQ* receive_buffer, PACKAGE_HEADER_ACK* transmit_buffer)
{
    if (receive_buffer->Version == PROTOCOL_VERSION) {
        switch (receive_buffer->Command) {
        case COMMAND_PING:
            transmit_buffer->Size = CommandPing(receive_buffer, transmit_buffer);
            break;
        case COMMAND_COPY_FILE_START:
            transmit_buffer->Size = CommandCopyFileStart(receive_buffer, transmit_buffer);
            break;
        case COMMAND_COPY_FILE_NEXT:
            transmit_buffer->Size = CommandCopyFileNext(receive_buffer, transmit_buffer);
            break;
        case COMMAND_COPY_FILE_END:
            transmit_buffer->Size = CommandCopyFileEnd(receive_buffer, transmit_buffer);
            break;
        case COMMAND_EXECUTE_COMMAND:
            transmit_buffer->Size = CommandExecuteCommand(receive_buffer, transmit_buffer);
            break;
        case COMMAND_EXECUTE_FILE:
            transmit_buffer->Size = CommandExecuteFile(receive_buffer, transmit_buffer);
            break;
        case COMMAND_IS_RUNNING:
            transmit_buffer->Size = CommandIsRunning(receive_buffer, transmit_buffer);
            break;

        case COMMAND_GET_PARTITION_BLOCK_SIZE:
            transmit_buffer->Size = CommandGetPartionsBlockSize(receive_buffer, transmit_buffer);
            break;
        case COMMAND_GET_PARTITION_SIZE:
            transmit_buffer->Size = CommandGetPartionsSize(receive_buffer, transmit_buffer);
            break;
        case COMMAND_READ_PARTITION_START:
            transmit_buffer->Size = CommandReadPartitionStart(receive_buffer, transmit_buffer);
            break;
        case COMMAND_READ_PARTITION_NEXT:
            transmit_buffer->Size = CommandReadPartitionNext(receive_buffer, transmit_buffer);
            break;
        case COMMAND_READ_PARTITION_END:
            transmit_buffer->Size = CommandReadPartitionEnd(receive_buffer, transmit_buffer);
            break;

        case COMMAND_CLOSE_CONNECTION:
            return 0;  // keien Response
            break;
        default:
            transmit_buffer->Command = receive_buffer->Command;
            sprintf(transmit_buffer->ErrorString, "wrong protocol version %i expected %i", receive_buffer->Version, transmit_buffer->Version);
            transmit_buffer->Size = sizeof(PACKAGE_HEADER_ACK) + strlen(transmit_buffer->ErrorString);
            break;
        }
    } else {
        transmit_buffer->Version = PROTOCOL_VERSION;
        transmit_buffer->Size = sizeof(PACKAGE_HEADER_ACK) + sprintf(transmit_buffer->ErrorString, "unknown command %i", transmit_buffer->Command);
        transmit_buffer->Ret = -1;
        transmit_buffer->Counter = receive_buffer->Counter;
    }
    return transmit_buffer->Size;
}


/*
* This will handle connection for each client
* */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    char *receive_buffer;
    char *transmit_buffer;
    uint32_t buffer_pos;
    uint32_t receive_message_size;
    uint32_t transmit_message_size;
    int len;

    receive_buffer = /*not my_*/ malloc(BUFFER_SIZE);
    transmit_buffer = /*not my_*/ malloc(BUFFER_SIZE);

    while (1) {
        buffer_pos = 0;
        receive_message_size = 0;
        do {
            len = recv(sock, receive_buffer + buffer_pos, BUFFER_SIZE - buffer_pos, 0);
            if (len > 0) {
                buffer_pos += len;
                if (receive_message_size == 0) receive_message_size = *(uint32_t*)receive_buffer;
            }
            else {
                printf("Failed receiving\n");
            }
        } while (buffer_pos < receive_message_size);

        transmit_message_size = DecodeCommand((PACKAGE_HEADER_REQ*)receive_buffer, (PACKAGE_HEADER_ACK*)transmit_buffer);

        if (transmit_message_size == 0) {
            printf("\nConnection should be closed\n");
            break;   // Thread beenden
        }
        buffer_pos = 0;
        *(uint32_t*)transmit_buffer = transmit_message_size;
        do {
            len = send(sock, transmit_buffer + buffer_pos, transmit_message_size - buffer_pos, 0);
            if (len > 0) {
                buffer_pos += len;
            }
            else {
                printf("Failed transmiting\n");
            }
        } while (buffer_pos < transmit_message_size);
    }
    close(sock);
    return 0;
}


int main(int argc, char *argv[])
{
    int value;
    int daemonize_flag = 0;
	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;
    FILE *fh = NULL; 

    if (fh != NULL) if (fh != NULL) fprintf(fh, "\nStarted\n");

    while ((value = getopt(argc, argv, "d")) > 0) {
        switch (value) {
        case 'd':
            daemonize_flag = 1;
            break;
        default:
            break;
        }
    }
    if (daemonize_flag) {
        if (fh != NULL) fprintf(fh, "try to daemonize\n");
        daemon(0, 0);
        if (fh != NULL) fprintf(fh, "should be a daemon now\n");
    }

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
        if (fh != NULL) fprintf(fh, "error: could not create socket!\n");
        return 1;
	}
    if (fh != NULL) fprintf(fh, "Socket created\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8890);

	//Bind
	if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		//print the error message
        if (fh != NULL) fprintf(fh, "error: bind failed\n");
		return 1;
	}
    if (fh != NULL) fprintf(fh, "bind done\n");

	//Listen
	listen(socket_desc, 3);

	//Accept and incoming connection
    if (fh != NULL) fprintf(fh, "Waiting for incoming connections...\n");
	c = sizeof(struct sockaddr_in);
	pthread_t thread_id;


	while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        if (fh != NULL) fprintf(fh, "Connection accepted\n");

		if (pthread_create(&thread_id, NULL, connection_handler, (void*)&client_sock) < 0) {
            if (fh != NULL) fprintf(fh, "error: could not create thread\n");
			return 1;
		}
        if (fh != NULL) fprintf(fh, "Handler assigned\n");
	}

	if (client_sock < 0) {
        fprintf(fh, "error: accept failed\n");
		return 1;
	}
	return 0;
}
