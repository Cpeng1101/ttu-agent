/*********************************************
*
*
*
*********************************************/
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "ttu-common.h"
#include "ttu-manage.h"
#include "ttu-client.h"

#define 	IMG_MAX_LEN	1300
char *image_info[IMG_MAX_LEN];
extern int sock_cli;

static int app_info_encode(char *buf, struct app_info *app_info)
{

	printf("app_info_encode enter !!!\n");
	ttu_hdr * ttu = (ttu_hdr*)buf;
	app_hdr * app = (app_hdr*)(buf + TTU_HDR_LEN);
	int app_fd = 0;
	int len = 0;

	memset(buf, 0, BUFFER_SIZE);
	ttu->OpCode = CREATE_APP;
	ttu->MsgCode = REQUEST;
	app->Off_name = 0;
	app->Off_paras = app->Off_name + strlen(app_info->name) + 1;
	app->Off_dname = app->Off_paras + strlen(app_info->paras) + 1;
	app->Off_desc = app->Off_dname + strlen(app_info->dname) + 1;
	app->Off_image = app->Off_desc + strlen(app_info->desc) + 1;
	app->first = 1;
	app->last = 0;
	app->flag = APP_REQUEST;
	app->data_size = IMG_MAX_LEN < app_info->image_size ? IMG_MAX_LEN : app_info->image_size;
	
	memcpy(buf + HDR_LEN_SUM, app_info->name, strlen(app_info->name));
	memset(buf + HDR_LEN_SUM + app->Off_paras - 1, 0, 1);
	memcpy(buf + HDR_LEN_SUM + app->Off_paras, app_info->paras, strlen(app_info->paras));
	memset(buf + HDR_LEN_SUM + app->Off_dname - 1, 0, 1);
	memcpy(buf + HDR_LEN_SUM + app->Off_dname, app_info->dname, strlen(app_info->dname));
	memset(buf + HDR_LEN_SUM + app->Off_desc - 1, 0, 1);
	memcpy(buf + HDR_LEN_SUM + app->Off_desc, app_info->desc, strlen(app_info->desc));
	memset(buf + HDR_LEN_SUM + app->Off_image - 1, 0, 1);

	app_info->image = buf + TTU_HDR_LEN + APP_HDR_LEN + app->Off_image;
	
	return 0;
}

// cl : 发出创建CREATE_APP消息后，等待对方反馈，再决定是否发送镜像文件
static int create_app_req(char *buf, struct app_info *app_info)
{
	printf("create_app_req enter !!!\n");
	ttu_hdr * ttu = (ttu_hdr*)buf;
	app_hdr * app = (app_hdr*)(buf + TTU_HDR_LEN);
	int app_fd = 0;
	int len = IMG_MAX_LEN;
	int hdr_len, data_size, image_size;
	struct stat stat_buf;

	if (stat(app_info->name, &stat_buf)){
		printf("ERROR: app_name : %s, %d\n", app_info->name, errno);
		return -1;
	}

	image_size = app_info->image_size = stat_buf.st_size;
	app_info_encode(buf, app_info);
	data_size = app->data_size;
	hdr_len = HDR_LEN_SUM + app->Off_image;

	app_fd = open(app_info->name, O_RDONLY);
	if (app_fd < 0)
		return -1;
	
	send(sock_cli, buf, hdr_len, 0);
	recv(sock_cli, buf, hdr_len, 0);
	
	if (app->flag == APP_EXIST) {
		printf("The agent already has the app : %s, exit...\n", app_info->name);
		close(app_fd);
		return APP_EXIST;
	} else if (app->flag == APP_ACK) {
		data_size = app->data_size < data_size  ?  app->data_size : data_size;
		app->flag = APP_DATA;
		printf("Start transport the app : %s, img_size %d\n", app_info->name, data_size);
	}
		
	int test = 0;
	while (len > 0) {
		
		if (image_size < data_size) {
			data_size = image_size;
			app->last = 1;
		}
		memset(app_info->image, 0, data_size);
		len = read(app_fd, app_info->image, data_size);
		//memset(app_info->image, 0, data_size);
		//memcpy(app_info->image, image_info, len);
		send(sock_cli, buf, hdr_len + len, 0);
		image_size -= data_size;
		printf("send!!!!!! %d !!! len = %d !!!!\n", test++, hdr_len + len);
		if (len < IMG_MAX_LEN) {
			break;
		}
	}

	printf("send ok!!!\n");
	return 0;
}

static int app_info_decode(char *buf, struct app_info *app_info)
{
	ttu_hdr * ttu = (ttu_hdr*)buf;
	app_hdr * app = (app_hdr*)(buf + TTU_HDR_LEN);
	char *data_start = buf + HDR_LEN_SUM;

	if (strlen(data_start + app->Off_name))
		memcpy(app_info->name, data_start + app->Off_name, strlen(data_start + app->Off_name));
	if (strlen(data_start + app->Off_paras))
		memcpy(app_info->paras, data_start + app->Off_paras, strlen(data_start + app->Off_paras));
	if (strlen(data_start + app->Off_dname))
		memcpy(app_info->dname, data_start + app->Off_dname, strlen(data_start + app->Off_dname));
	if (strlen(data_start + app->Off_desc))
		memcpy(app_info->desc, data_start + app->Off_desc, strlen(data_start + app->Off_desc));

	return 0;
}

static int create_app_do(char *buf, struct app_info *app_info)
{
	printf("create_app_do enter !!!!\n");
	app_hdr * app = (app_hdr*)(buf + TTU_HDR_LEN);
	int app_fd = 0;
	int len = 0;
	char *image = buf + HDR_LEN_SUM;
	int buf_len;

	app_info_decode(buf, app_info);

	if (app->flag == APP_REQUEST) {
		
		app_fd = open(app_info->name, O_WRONLY | O_CREAT);
		if (app_fd < 0) {
			printf("Open file failed appname %s, paras %s\n", app_info->name, app_info->paras);
			app->flag = APP_EXIST;
		} else {
			printf("Open file success appname %s, paras %s\n", app_info->name, app_info->paras);
			app->flag = APP_ACK;
		}
		app->data_size = app->data_size < IMG_MAX_LEN ? app->data_size : IMG_MAX_LEN;
		len = HDR_LEN_SUM + app->Off_image;
		send(sock_cli, buf, len, 0);
		buf_len = app->data_size + HDR_LEN_SUM + app->Off_image;
		int i = 0;
		for (; i < len; i++) {
			printf("%02x", buf[i]);
		}
	} else {
		return 0;
	}
	
	while (len > 0) {
		len = recv(sock_cli, buf, buf_len, 0);
		printf("len = %d!!!\n", len);
		write(app_fd, image + app->Off_image, len - TTU_HDR_LEN - APP_HDR_LEN - app->Off_image);
		if (app->last == 1)
			break;
		memset(buf, 0, BUFFER_SIZE);
	}

	printf("recv ok!!!\n");
	return 0;
}

static int stop_app_req(char *buf, struct app_info *app_info)
{
	return 0;
}


static int stop_app_do(char *buf, struct app_info *app_info)	
{
	return 0;
}

static int delete_app_req(char *buf, struct app_info *app_info)
{
	return 0;
}


static int delete_app_do(char *buf, struct app_info *app_info)	
{
	return 0;
}

static int upgrade_app_req(char *buf, struct app_info *app_info)
{
	return 0;
}


static int upgrade_app_do(char *buf, struct app_info *app_info)	
{
	return 0;
}

static int show_all_apps_req(char *buf, struct app_info *app_info)
{
	return 0;
}


static int show_all_apps_do(char *buf, struct app_info *app_info)	
{
	return 0;
}

static int show_one_app_req(char *buf, struct app_info *app_info)
{
	return 0;
}

static int show_one_app_do(char *buf, struct app_info *app_info)	
{
	return 0;
}

static int show_one_ttu_req(char *buf, struct app_info *app_info)
{
	return 0;
}

static int show_one_ttu_do(char *buf, struct app_info *app_info)	
{
	return 0;
}

static int upgrade_ttu_req(char *buf, struct app_info *app_info)
{
	return 0;
}

static int upgrade_ttu_do(char *buf, struct app_info *app_info)	
{
	return 0;
}

struct cmd_ops cmd[MAX_ORDER_NUM] = {
	[CREATE_APP] = {
		.create_req = create_app_req,
		.recv_req = create_app_do,	
		},
	[STOP_APP] = {
		.create_req = stop_app_req,
		.recv_req = stop_app_do,	
		},
	[DELETE_APP] = {
		.create_req = delete_app_req,
		.recv_req = delete_app_do,	
		},
	[UPGRADE_APP] = {
		.create_req = upgrade_app_req,
		.recv_req = upgrade_app_do,	
		},
	[SHOW_ALL_APPS] = {
		.create_req = show_all_apps_req,
		.recv_req = show_all_apps_do,	
		},
	[SHOW_ONE_APP] = {
		.create_req = show_one_app_req,
		.recv_req = show_one_app_do,	
		},
	[SHOW_ONE_TTU] = {
		.create_req = show_one_ttu_req,
		.recv_req = show_one_ttu_do,	
		},
	[UPGRADE_TTU] = {
		.create_req = upgrade_ttu_req,
		.recv_req = upgrade_ttu_do,	
		},
};

