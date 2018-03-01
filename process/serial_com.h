#ifndef __SREIAL_COM_H__
#define __SREIAL_COM_H__

//宏定义    
//#define FALSE  -1    
//#define TRUE   0    

#define BAUD_RATE  115200
#define BUFFER_LEN	128
#define SERIAL_DEV	"/dev/ttyS1"

typedef struct {
	char sof[2];	//start of frame
	char lof[2];	//length of frame
	char reserve[4];
	char cmd[2];
	char checksum;
	char eof[2];	
} serial_frame;

typedef struct {
	int faceid;
	float confidence;
} result_of_recog; //result of recognition

/*******************************************************************  
* 名称：                  UART_Open  
* 功能：                打开串口并返回串口设备文件描述  
* 入口参数：        fd    :文件描述符      port :串口号(ttyS0,ttyS1,ttyS2)  
* 出口参数：        正确返回为1，错误返回为0  
*******************************************************************/    
int UART_Open(int fd,char* port);
    
/*******************************************************************  
* 名称：                UART_Close  
* 功能：                关闭串口并返回串口设备文件描述  
* 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)  
* 出口参数：        void  
*******************************************************************/    
     
void UART_Close(int fd);
     
/*******************************************************************  
* 名称：                UART_Set  
* 功能：                设置串口数据位，停止位和效验位  
* 入口参数：        fd        串口文件描述符  
*                              speed     串口速度  
*                              flow_ctrl   数据流控制  
*                           databits   数据位   取值为 7 或者8  
*                           stopbits   停止位   取值为 1 或者2  
*                           parity     效验类型 取值为N,E,O,,S  
*出口参数：          正确返回为1，错误返回为0  
*******************************************************************/    
int UART_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);

/*******************************************************************  
* 名称：                UART_Init()  
* 功能：                串口初始化  
* 入口参数：        fd       :  文件描述符     
*               speed  :  串口速度  
*                              flow_ctrl  数据流控制  
*               databits   数据位   取值为 7 或者8  
*                           stopbits   停止位   取值为 1 或者2  
*                           parity     效验类型 取值为N,E,O,,S  
*                        
* 出口参数：        正确返回为1，错误返回为0  
*******************************************************************/    
int UART_Init(int fd, int speed,int flow_ctrl,int databits,int stopbits,int parity);
    
     
/*******************************************************************  
* 名称：    	UART_Recv  
* 功能：    	接收串口数据  
* 入口参数：fd                  :文件描述符      
* rcv_buf     :接收串口中数据存入rcv_buf缓冲区中  
* data_len    :一帧数据的长度  
* 出口参数：        正确返回为1，错误返回为0  
*******************************************************************/    
int UART_Recv(int fd, char *rcv_buf,int data_len);
    
/********************************************************************  
* 名称：                  UART_Send  
* 功能：                发送数据  
* 入口参数：        fd                  :文件描述符      
*                              send_buf    :存放串口发送数据  
*                              data_len    :一帧数据的个数  
* 出口参数：        正确返回为1，错误返回为0  
*******************************************************************/    
int UART_Send(int fd, char *send_buf,int data_len);

#endif

