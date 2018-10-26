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

#include "ttu-common.h"
#include "ttu-client.h"

extern char recv_data[BUFFER_SIZE];
extern char send_data[BUFFER_SIZE];
int sock_cli;
int ttu_socket_recv(char *buf, int len)
{
	ttu_hdr * hdr = (ttu_hdr *)buf;

	switch (hdr->MsgCode) {
	case ACK: break;
	case NACK:	break;
	case REQUEST:
		switch (hdr->OpCode) {
		case CREATE_APP:
		case STOP_APP:
		case DELETE_APP:
		case UPGRADE_APP:
		case SHOW_ALL_APPS:
		case SHOW_ONE_APP:
		case SHOW_ONE_TTU:
		case UPGRADE_TTU:
				break;
		case MONITOR_APP_RULE:
		case GET_APP_STATE:
		case GET_APP_RES_INFO:
				break;
		}
		break;
	default:
		break;
	};
	return 0;
}

extern int recv_data_by_socket(char *buf, struct app_info *app_info);
void * ttu_thread_entry(void *arg)
{
	//int sock_cli;
	struct sockaddr_in agent_addr;
	fd_set ttu_set;
	char buf[1024];
	int len;
	int ret;
	int pipes[2];
	char *test = "hello, world\n";
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
	
	sock_cli = socket(AF_INET, SOCK_STREAM, 0);
	memset(&agent_addr, 0, sizeof(agent_addr));
	agent_addr.sin_family = AF_INET;
	agent_addr.sin_port = htons(AGENT_PORT);
	agent_addr.sin_addr.s_addr = inet_addr(AGENT_IP);

	if (connect(sock_cli, (struct sockaddr*)&agent_addr, sizeof(agent_addr)) < 0) {
		printf("ERROR: connect err, agent_ip: %s, agent_port:%d", AGENT_IP, AGENT_PORT);
		exit(-1);
	}

	printf("Connect establish!!!\n Agent IP: %s, Agent port %d", AGENT_IP, AGENT_PORT);
	while (1) {
		FD_ZERO(&ttu_set);
		FD_SET(sock_cli, &ttu_set);
		FD_SET(pipes[0], &ttu_set);
	
		ret = select((sock_cli > pipes[0] ? sock_cli : pipes[0]) + 1, &ttu_set, NULL, NULL, NULL);

		if (ret < 0) {
			if (ret == -EAGAIN) {
				continue;
			} else {
				printf("Errno, select err errno: %d\n", ret);
				exit(-1);
			}
		}
		
		if (FD_ISSET(sock_cli, &ttu_set)) {
			FD_CLR(sock_cli, &ttu_set);
			memset(recv_data, 0, BUFFER_SIZE);
			len = recv(sock_cli, recv_data, BUFFER_SIZE, 0);
			printf("Order from agent, %d\n", len);
			write(pipes[0], recv_data, len);
			if (len > 0)
				recv_data_by_socket(recv_data, &app_info);
			//	write(pipes[0], "1", 1);
			else if (len == 0) {
				printf("Agent has closed the connection!\n");
				close(sock_cli);
				break;
			}	
		} else if (FD_ISSET(pipes[0], &ttu_set)) {
			len = read(pipes[0], send_data, BUFFER_SIZE);
			printf("Order from manager,send to agent len : %d\n", len);
			len = send(sock_cli, send_data, len, 0);
			memset(send_data, 0, len);
		}
	}
}