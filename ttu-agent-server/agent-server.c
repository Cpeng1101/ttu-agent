/*********************************************
*
*
*
*********************************************/


#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <errno.h>
#include <pthread.h>

#include "agent-common.h"
#include "agent-server.h"

int conn;

extern char recv_data[BUFFER_SIZE];
extern char send_data[BUFFER_SIZE];
int ttu_socket_recv(char *buf, int len)
{
	ttu_hdr * hdr = (ttu_hdr *)buf;

	switch (hdr->MsgCode) {
	case ACK: break;
	case NACK:	break;
	};
	return 0;
}

extern int recv_data_by_socket(char *buf, struct app_info *app_info);
void * ttu_thread_entry(void *arg)
{
	int sock_ser;
	struct sockaddr_in agent_addr;
	struct sockaddr_in ttu_addr;
	socklen_t length = sizeof(ttu_addr);
	fd_set ttu_set;
	char buf[1024];
	int len;
	int ret;
	int pipes[2];
	char *test = "hello, world\n";
//	int conn;
	struct app_info app_info;
	char app_name[20] = "";
	char paras[20] = "";
	char dname[20] = "";
	char desc[20] = "";
	
	app_info.name = app_name;
	app_info.paras = paras;
	app_info.dname = dname;
	app_info.desc = desc;
	pthread_detach(pthread_self());

	pipes[0] = *(int*)arg;
	pipes[1] = *((int*)arg + 1);
	
	sock_ser = socket(AF_INET, SOCK_STREAM, 0);
	memset(&agent_addr, 0, sizeof(agent_addr));
	agent_addr.sin_family = AF_INET;
	agent_addr.sin_port = htons(AGENT_PORT);
	agent_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock_ser, (struct sockaddr*)&agent_addr, sizeof(agent_addr)) < 0) {
		printf("ERROR: bind err, agent_ip: %s, agent_port:%d", AGENT_IP, AGENT_PORT);
		exit(-1);
	}

	if (listen(sock_ser, 5) < 0) {
		printf("ERROR: listen err\n");
		exit(-1);
	}

	if ((conn = accept(sock_ser, (struct sockaddr*)&ttu_addr, &length)) < 0) {
		printf("ERROR: connect err\n");
		exit(-1);
	}
	
	printf("Connect establish!!!\n");
	
	while (1) {
		FD_ZERO(&ttu_set);
		FD_SET(conn, &ttu_set);
		FD_SET(pipes[0], &ttu_set);
	
		ret = select((conn > pipes[0] ? conn : pipes[0]) + 1, &ttu_set, NULL, NULL, NULL);

		if (ret < 0) {
			if (ret == -EAGAIN) {
				continue;
			} else {
				printf("Errno, select err errno: %d\n", ret);
				exit(-1);
			}
		}
		
		if (FD_ISSET(conn, &ttu_set)) {
			FD_CLR(conn, &ttu_set);
			int i = 0;
			memset(recv_data, 0 , BUFFER_SIZE);
			len = read(conn, recv_data, BUFFER_SIZE);
			printf("Order from ttu ,%d\n", len);
			for (; i < TTU_HDR_LEN + APP_HDR_LEN; i++)
				printf("%02x ", recv_data[i]);
			printf("\n");			
			if (len > 0)
				recv_data_by_socket(recv_data, &app_info);
			//	write(pipes[0], "1", 1);
			else if (len == 0) {
				printf("TTU has closed the connection!\n");
				close(conn);
				close(sock_ser);
				break;
			}	
		} else if (FD_ISSET(pipes[0], &ttu_set)) {
			len = read(pipes[0], send_data, BUFFER_SIZE);
			printf("Order from manager, send to client %s\n", buf);
			len = send(conn, send_data, len, 0);
		}
	}
}