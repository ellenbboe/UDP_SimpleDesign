#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#define WINDOW_SIZE 4
#define SERVER_PORT 8888
#define BUFF_LEN 1024
/*
 * 每2秒没有收到数据会减少一个接收窗口,初始化是2
 */
struct Pack{
    int start;
    int id;
    char buf[BUFF_LEN];//接收缓冲区，1024字节
};

struct ACK{
    int id;
    int count;
    int sackresend[WINDOW_SIZE];
    int sack;
};


struct funpara
{
    int fd;
    struct ACK ack;
    struct sockaddr_in sockaddrIn;
};



void *delaysend(void *arg){//延迟6秒发送
    sleep(6);
    struct funpara *para;
    para = (struct funpara *)arg;
    sendto(para->fd,&(para->ack), sizeof(para->ack),0,(struct sockaddr*)&para->sockaddrIn,sizeof(para->sockaddrIn));
}


void handle_udp_msg(int fd)
{
    struct Pack data[WINDOW_SIZE];
//    struct Pack data;//从客户端接受的数据
//    struct Pack Anotherdata;
    struct ACK ack;
    int i,j;
//    char buf[BUFF_LEN];
    socklen_t len;//表示指向内容的长度
    int count = 0;
    int sendid;
    int sendindex[WINDOW_SIZE]={-999};
    struct sockaddr_in clent_addr;  //clent_addr用于记录发送方的地址信息
    while(1)
    {
        int recv[WINDOW_SIZE]={-1};//初始化都是没有接收到的
        pthread_t pthread;
        len = sizeof(clent_addr);
        for(i=0;i<WINDOW_SIZE;i++)
        {
            memset(data[i].buf, 0, BUFF_LEN);
            recv[i] = recvfrom(fd, &data[i], sizeof(data[i]), 0, (struct sockaddr*)&clent_addr, &len);
        }
//        memset(data.buf, 0, BUFF_LEN);
//        memset(Anotherdata.buf, 0, BUFF_LEN);

        //得到收到的index
        for(i=0;i<WINDOW_SIZE;i++)
        {
            if(recv[i]!= -1)
            {
                int index = data[i].id - data[i].start;//已经收到的在客户端缓存表中的index
                sendindex[i] = index;
            }
        }
        //去重
        for(i=0;i<WINDOW_SIZE;i++)
        {
            if(sendindex[i]!=-999)
            {
                int index = sendindex[i];
                for(j = i+1;j<WINDOW_SIZE;j++){
                    if(sendindex[j] == index){
                        sendindex[j] = -999;
                    }
                }
            }
        }

        for(i=0;i<WINDOW_SIZE;i++){
            if(sendindex[i]!=-999)
            {
                count++;
            }
        }


//        if(count != -1 && count1 != -1)//都收到了
//        {
//            if(data.id == Anotherdata.id && data.id!=data.start){
//                printf("收到两个重复的不是开始的包,窗口已满,丢弃重新接受\n");
//                continue;
//            }else if(data.id == Anotherdata.id && data.id ==data.start){
//                printf("收到两个开始的包\n丢弃重复的包,窗口下移\n");
//                sendid=data.id;
//            }else{
//                printf("接收到id为%d的客户端消息:%s\n",data.id,data.buf);  //打印client发过来的信息
//                printf("接收到id为%d的客户端消息:%s\n",Anotherdata.id,Anotherdata.buf);  //打印client发过来的信息
//                sendid = data.id>Anotherdata.id? data.id:Anotherdata.id;
//            }
//        }else if(count != -1 && count1 == -1)//只收到了一个
//        {
//            if(data.id == data.start)
//            {
//                sendid = data.id;
//            }else{
//                printf("丢失一个,且收到一个不符合期望的包,丢弃重新接收\n");
//                continue;
//            }
//        }else if(count1 == -1 && count == -1){
//            printf("未在规定时间内发送数据,初始化窗口,重新接收\n");
//            continue;
//        }
        //服务端发送消息
        memset(&ack, 0, sizeof(ack));//初始化一下

        if(count == 0)
        {
            //不发送
        }else{
            if(count==WINDOW_SIZE)
            {
                ack.id = data[0].start+WINDOW_SIZE-1;
                ack.sack = 0;
            }else{
                ack.id = -999;//由于重传的id在数组中已经确认,这里就随便写了
                ack.sack = 1;
                for(i=0;i<WINDOW_SIZE;i++)
                {
                    ack.sackresend[i] = sendindex[i];
                }
                ack.count = count;
            }
        }
        count = 0;
//        ack.id = sendid;
//        sprintf(data.buf, "I have recieved %d bytes data!\n", count);  //回复client(复制信息)
//        printf("%d,server:%s\n",flag,data.buf);  //打印自己发送的信息
        if(ack.sack ==0){
            printf("发送ACK的ID:%d\n",ack.id);
        } else{
            printf("需要客户端重新传丢失包\n");
        }

        //服务端发送信息,发送ack消息

        int randomsum = rand()%1500+1;//增加随机性
        printf("随机数:%d\n",randomsum);

        if(randomsum<1000){//正常接受
            if(ack.sack == 0){
                printf("发送ACK(全部接收成功)\n\n");
                sendto(fd, &ack, sizeof(ack),0, (struct sockaddr*)&clent_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针
            }else{
                sendto(fd, &ack, sizeof(ack),0, (struct sockaddr*)&clent_addr, len);
                printf("接受未收到的包\n\n");
                int count = ack.count;
                    for(i = 0;i<count;i++)
                    {
                        memset(&ack, 0, sizeof(ack));//初始化一下
                        int result = recvfrom(fd, &data[i], sizeof(data[i]), 0, (struct sockaddr*)&clent_addr, &len);
                        if(result != -1){
                            ack.sack = 0;
                            ack.id = data[i].id;
                            sendto(fd, &ack, sizeof(ack),0, (struct sockaddr*)&clent_addr, len);
                        }
                    }
            }
        }else if(randomsum>=1000 && randomsum<1250 && ack.sack == 0){//延迟发送
            printf("ACK延迟发送\n\n");
            struct funpara funpara1;
            funpara1.fd = fd;
            funpara1.ack=ack;
            funpara1.sockaddrIn=clent_addr;
            pthread_create(&pthread,NULL,delaysend,&funpara1);
//            sleep(5);
//            sendto(fd, &ack, sizeof(ack), 0, (struct sockaddr*)&clent_addr, len);
        }
//        else if(randomsum>=1250){//模拟只接受到了一个包,其实在前面已经控制了只接受了一个的情况,但是由于接收时都是全部接收到的,所以难以出现
//            printf("发送ACK(只接收到了一个ack的情况)\n\n");
//            ack.id--;
//            sendto(fd, &ack, sizeof(ack),0, (struct sockaddr*)&clent_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针
//        }//注释原因:由于加入sack机制,可以重传单独的包
        else if(ack.sack == 0){
            printf("丢弃ACK\n\n");//没有收到开始的包或者是没有收到包或者是Ack丢失
        }
//        !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!break;//
//        sendto(fd, &ack, sizeof(ack), 0, (struct sockaddr*)&clent_addr, len);  //发送信息给client，注意使用了clent_addr结构体指针
    }

}

/*
    server:
            socket-->bind-->recvfrom-->sendto-->close
*/

int main(int argc, char* argv[])
{
    srand(time(NULL));
    int server_fd, ret;
    struct sockaddr_in ser_addr;//服务端的地址信息
    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP   //文件描述符
    if(server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }
    memset(&ser_addr, 0, sizeof(ser_addr));//初始化赋值为0
    ser_addr.sin_family = AF_INET;//ipv4协议
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);  //端口号，需要网络序转换
    ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));//将socket绑定到ip和端口上
    if(ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }
//    struct timeval tv={3,0};
//    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


    handle_udp_msg(server_fd);   //处理接收到的数据
    close(server_fd);
    return 0;
}