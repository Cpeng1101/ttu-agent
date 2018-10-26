/*********************************************
*
*
*
*********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ttu-common.h"
#include "ttu-manage.h"
#include "ttu-client.h"

#define	STDIN	0
#define	MANAGE_USER_RBUF_LEN	50
extern struct cmd_ops cmd[MAX_ORDER_NUM];

static char *order_arr[MAX_ORDER_NUM] = {
	"Order from user: CREATE_APP\n",
	"Order from user: STOP_APP\n",
	"Order from user: DELETE_APP\r\n",
	"Order from user: UPGRADE_APP\r\n",
	"Order from user: SHOW_ALL_APPS\r\n",
	"Order from user: SHOW_ONE_APP\r\n",
	"Order from user: SHOW_ONE_TTU\r\n",
	"Order from user: UPGRADE_TTU\r\n",
};

static char manage_user_rbuf[MANAGE_USER_RBUF_LEN];
//static char manage_user_wbuf[];
static char buff[1024];

char recv_data[BUFFER_SIZE];
char send_data[BUFFER_SIZE];

static int create_ttu_thread(int *pipes)
{
	pthread_t ttu_thread;
	pthread_attr_t attr; 

	int ret;
	
	pthread_attr_init(&attr);
 	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&ttu_thread, &attr, ttu_thread_entry, (void *)pipes);
	
	if (ret) {
		printf("ERROR: create_ttu_thread failed, errno: %d\n", ret);
	}
	
	return ret;
}

static int send_data_by_socket(char *buf, int order, struct app_info *app_info)
{
	memset(buf, 0, BUFFER_SIZE);
	cmd[order].create_req(buf, app_info);
}


int recv_data_by_socket(char *buf, struct app_info *app_info)
{
	ttu_hdr * hdr = (ttu_hdr *)buf;

	switch (hdr->MsgCode) {
	case ACK: break;
	case NACK:	break;
	case REQUEST:
		cmd[hdr->OpCode].recv_req(buf, app_info);
	default:
		break;
	};
	return 0;
}

static int getopt_order(char *order, char *app_name, char *paras, int len)
{
	int i = 0;
	int num = 0;
	int mark = 0;
	int j = 0 , k = 0;
	
	while (*(order + i) != 0 && i < len) {

		if (*(order + i) == '\r' || *(order + i) == '\n')
			break;
		
		if (*(order + i) == ' ' && *(order + i - 1) != ' ' && mark <= 1) {
			mark ++;
			i++;
			continue;
		}
		if (mark == 0 && *(order + i) >= '0' && *(order + i) <= '9')
			num = num * 10 + *(order + i) - '0';
		else if (mark == 1) {
			*app_name = *(order + i);
			app_name++;
			j ++;
		} else {
			*paras = *(order + i);
			paras++;
			k++;
		}	
		i++;
	}
	printf("j = %d k = %d\n", j, k);
	return num;
}

int main()
{
	int fd_sock;
	long num;
	int ret;
	int len = 0;
	fd_set manage_set;
	int pipes[2];
	char app_name[20] = "";
	char paras[20] = "";
	char dname[20] = "teat-dname";
	char desc[20] = "test-desc";
	struct app_info app_info;

	app_info.name = app_name;
	app_info.paras = paras;
	app_info.dname = dname;
	app_info.desc = desc;
	
	socketpair(AF_UNIX, SOCK_STREAM, 0, pipes);
	
	if (create_ttu_thread(pipes)) {
		close(pipes[0]);
		close(pipes[1]);
		return ret;
	}

	while (1) {
		FD_ZERO(&manage_set);
		FD_SET(STDIN, &manage_set);
		FD_SET(pipes[1], &manage_set);
		ret = select(pipes[1] + 1, &manage_set, NULL, NULL, NULL);

		if (ret < 0) {
			if (ret == -EAGAIN) {
				printf("ERROR: select err errno: %d\n", ret);
				continue;
			} else {
				printf("ERROR: select err errno: %d\n", ret);
				exit(-1);
			}
		}
		
		if (FD_ISSET(STDIN, &manage_set))
		{
			memset(manage_user_rbuf, 0, MANAGE_USER_RBUF_LEN);
			len = read(STDIN, manage_user_rbuf, MANAGE_USER_RBUF_LEN);
			if (len > 0) {
				num = getopt_order(manage_user_rbuf, app_name, paras, len);
				printf("order %d,app name %s, paras %s\n", num, app_name, paras);
				if (num >0 && num <= MAX_ORDER_NUM) {
					send_data_by_socket(send_data, num, &app_info);
					printf("Order send to agent: %s\n", order_arr[num - 1]);
				}
			}
		} else if (FD_ISSET(pipes[1], &manage_set)) {
			len = read(pipes[1], buff, BUFFER_SIZE);
			recv_data_by_socket(recv_data, &app_info);
			printf("Order recv from agent: %s\n", buff);
		}
	};
	return 0;
}

