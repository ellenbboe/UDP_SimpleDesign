#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define SERVER_PORT 8888
#define BUFF_LEN 512
#define WINDOW_SIZE 4
#define SERVER_IP "172.0.5.182"
struct Pack{
    int start;
    int id;
    char buf[BUFF_LEN];
};


struct ACK{
    int id;
    int count;
    int sackresend[WINDOW_SIZE];
    int sack;
};


void udp_msg_sender(int fd, struct sockaddr* dst)
{
    //dst是服务端的地址和端口
    int i;
    socklen_t len;//指向内容的长度
    struct sockaddr_in src;//新建对象就收发送过来的ip地址和端口
    int flag = 1;//发送包的id

    int blocktime = 1;//超时就断开连接
    struct Pack data[WINDOW_SIZE];
    while(1)
    {

//        struct Pack data;
//        struct Pack Anotherdata;
        int start = flag;
        struct ACK ack;
//        Pack.buf = "TEST UDP MSG!\n";
        for(i = 0;i<WINDOW_SIZE;i++)
        {
            strcpy(data[i].buf,"TEST UDP MSG!");
            data[i].id = flag++;//设置包的flag
            data[i].start = start;
        }
        len = sizeof(*dst);
        for(i = 0;i<WINDOW_SIZE;i++)
        {
            printf("发送ID %d,客户端消息:%s\n",data[i].id,data->buf);  //打印自己发送的信息
        }
//        Anotherdata.start = data.id;
        for(i = 0;i<WINDOW_SIZE;i++)
        {
            sendto(fd, &data[i], sizeof(data[i]), 0, dst, len);//发送自己的消息
        }
//        sendto(fd, &data, sizeof(data), 0, dst, len);//发送自己的消息
//        sendto(fd, &Anotherdata, sizeof(Anotherdata), 0, dst, len);//发送自己的消息

/*
 * 接收
 */


        memset(&ack, 0, sizeof(ack));//初始化一下
//        signal(SIGALRM,sendto(fd, &data, sizeof(data), 0, dst, len));
//        alarm(2);//5秒超时
        int b = recvfrom(fd, &ack, sizeof(ack), 0, (struct sockaddr*)&src, &len);//接收来自server的信息

        while(1){
            if(b != -1)
            {
                if(ack.sack == 1)//需要单传
                {
                    struct Pack resendData[ack.count];
                    int index=0;
                    for(i = 0;i<WINDOW_SIZE;i++)
                    {
                        if(ack.sackresend[i] != -999)
                        {
                            resendData[index++] = data[ack.sackresend[i]];
                        }
                    }
                    for(i = 0;i<ack.count;i++)
                    {
                        resendData[i].start = resendData[i].id;//单独传
                        sendto(fd, &resendData[i], sizeof(resendData[i]), 0, dst, len);//重新发送自己的消息
                        int c = recvfrom(fd, &ack, sizeof(ack), 0, (struct sockaddr*)&src, &len);
                        while(c == -1){
                            sendto(fd, &resendData[i], sizeof(resendData[i]), 0, dst, len);//重新发送自己的消息
                            c = recvfrom(fd, &ack, sizeof(ack), 0, (struct sockaddr*)&src, &len);
                        }
                        printf("重发%d完成 (已收到)\n",ack.id);
                    }

                }else if(ack.sack == 0 && ack.id==data[0].start+WINDOW_SIZE-1)//全收到了
                {
                    printf("接收到了匹配的ACK返回,id为%d\n\n",ack.id);
                    break;
                }

            }
            if(b!= -1 && ack.id!=data[0].start+WINDOW_SIZE-1 && ack.id != -999){
                printf("收到的ack的id与期望不符合,丢弃并重新接收,id为%d\n\n",ack.id);
                b = recvfrom(fd, &ack, sizeof(ack), 0, (struct sockaddr*)&src, &len);
            }

            if(b == -1 && blocktime<10)//重传机制 4秒传一次
            {
                for(i = 0;i<WINDOW_SIZE;i++)
                {
                    sendto(fd, &data[i], sizeof(data[i]), 0, dst, len);//重新发送自己的消息
                    printf("重传次数%d,id为%d的包\n",blocktime,data[i].id);
                }
                blocktime++;
//
//                sendto(fd, &Anotherdata, sizeof(Anotherdata), 0, dst, len);//发送自己的消息
//                printf("重传次数%d,id为%d的包\n",blocktime++,Anotherdata.id);
                b = recvfrom(fd, &ack, sizeof(ack), 0, (struct sockaddr*)&src, &len);
            }

            if(blocktime == 10){
                printf("传输超时,断开连接\n");
                break;
            }
        }
        if(blocktime == 10){
            break;
        }
//        alarm(0);
        blocktime = 1;

        sleep(1);  //一秒发送一次消息
    }
}

/*
    client:
            socket-->sendto-->revcfrom-->close
*/

int main(int argc, char* argv[])
{
    int client_fd;
    struct sockaddr_in ser_addr;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);//文件描述符

    if(client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    //ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //注意网络序转换
    ser_addr.sin_port = htons(SERVER_PORT);  //注意网络序转换
    struct timeval tv={4,0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    udp_msg_sender(client_fd, (struct sockaddr*)&ser_addr);

    close(client_fd);//关闭描述符

    return 0;
}