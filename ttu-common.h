/*********************************************
*
*
*
*********************************************/

#ifndef __TTU_PROTOCOL_H__
#define __TTU_PROTOCOL_H__

#ifndef		uint8
#define		uint8	unsigned char
#endif

#ifndef		uint16
#define		uint16	unsigned short
#endif

#ifndef		uint32
#define 	uint32	unsigned int
#endif

typedef struct TTU_header 
{
	uint16	OpCode;
	uint8	MsgCode;
	uint8	reserved[5];
}ttu_hdr;

typedef struct APP_header
{
	uint8	Off_name;
	uint8	Off_paras;
	uint8	Off_dname;
	uint8	Off_desc;
	uint8	Off_image;
	uint16	data_size;
	uint8	first:1,
			last:1,
			flag:2;
}app_hdr;

struct app_info
{
	char *name;
	char *paras;
	char *dname;
	char *desc;
	char *image;
	int image_size;
};

struct cmd_ops
{
	int (*create_req)(char *buf, struct app_info *app_info);
	int (*recv_req)(char *buf, struct app_info *app_info);
};

	
#define APP_REQUEST	0
#define APP_DATA		1
#define APP_ACK			2
#define APP_EXIST		3

//OPCODE
#define	SHOW_ONE
#define UPLOAD
#define DELETE
#define SHOW_ALL

enum OPCODE {
	CREATE_APP = 1,
	STOP_APP,
	DELETE_APP,
	UPGRADE_APP,
	SHOW_ALL_APPS,
	SHOW_ONE_APP,
	SHOW_ONE_TTU,
 	UPGRADE_TTU,
	MONITOR_APP_RULE,
	GET_APP_STATE,
	GET_APP_RES_INFO,
	MAX_ORDER_NUM,
};

enum MSGCODE{
	REQUEST = 0,
	ACK,
	NACK,
};

#define	MONITOR_OS
#define	MONITOR_APP
#define MONITOR_INFO_SHOW
#define	WARNING_DELETE
#define APP_INFO_GET

#define	TTU_HDR_LEN	(sizeof(ttu_hdr))
#define	APP_HDR_LEN	(sizeof(app_hdr))
#define HDR_LEN_SUM	(TTU_HDR_LEN + APP_HDR_LEN)
#endif
