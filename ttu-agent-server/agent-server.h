/*********************************************
*
*
*
*********************************************/


#ifndef __TTU_SOCKET_H__
#define	__TTU_SOCKET_H__

#define	TTU_PORT
#define AGENT_PORT		1234
#define AGENT_IP		"127.0.0.1"
#define	BUFFER_SIZE	1480


/****************************************
* Func:	ttu_thread_entry
* Parameters:
* Return:
* Auther:
* Date:
*****************************************/
void * ttu_thread_entry(void *arg);

/****************************************
* Func:	ttu_socket_recv
* Parameters:
* Return:
* Auther:
* Date:
*****************************************/
int ttu_socket_recv(char *buf, int len);

/****************************************
* Func:	ttu_socket_send
* Parameters:
* Return:
* Auther:
* Date:
*****************************************/
int ttu_socket_send(char *buf, int len);

#endif
