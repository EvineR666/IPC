#include <pthread.h>
#include "comu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


// 向qid指定的消息队列发消息。
int _send(int qid, char *msg, long msgtype, int msg_sflags)
{
    // 把消息组装到结构体中
    msgmbuf msg_sbuf;
    memset(&msg_sbuf, 0, (size_t)sizeof(msg_sbuf));
    msg_sbuf.mtype = msgtype;
    strcpy(msg_sbuf.mtext, msg);
    // printf("msg: %s, type:%ld, len:%d , ", msg_sbuf.mtext, msg_sbuf.mtype, (int)strlen(msg));
    // printf("msg_sbuf.mtext: %s\n", msg_sbuf.mtext);
    
    // IPC_NOWAIT：消息队列满则立即返回，并将 errno 设置为 EAGAIN。
    int ret = msgsnd(qid, &msg_sbuf, (size_t)strlen(msg_sbuf.mtext), msg_sflags);

    return ret;
}


// 从qid指定的消息队列中接收消息。
int _receive(int qid, msgmbuf *rbuf, int rtype, int msg_rflags)
{
    size_t rcvsize = 256;
    int ret = msgrcv(qid, rbuf, rcvsize, rtype, msg_rflags);
    return ret;
}


void msg_show_attr(int msg_id, struct msqid_ds msg_info)
{
    int ret = -1;
    ret = msgctl(msg_id, IPC_STAT, &msg_info);
    if(ret == -1) {
        printf("Fails to obtain msgq info.\n");
    } else {
    printf("\n");
    printf("目前队列的字节数: %ld\n", msg_info.msg_cbytes);
    printf("目前队列的消息数: %d\n", (int)msg_info.msg_qnum);
    printf("队列的最大字节数: %d\n", (int)msg_info.msg_qbytes);
    printf("最后发送消息的进程PID: %d\n", msg_info.msg_lspid);
    printf("最后接收消息的进程PID: %d\n", msg_info.msg_lrpid);
    printf("最后发送消息的时间: %s", ctime(&(msg_info.msg_stime)));
    printf("最后接收消息的时间: %s", ctime(&(msg_info.msg_rtime)));
    printf("最后变化时间: %s", ctime(&(msg_info.msg_ctime)));
    printf("队列uid: %d\n", msg_info.msg_perm.uid);
    printf("消息gid: %d\n", msg_info.msg_perm.gid);
    printf("\n");
    }

    return;
}


// 进程A发送quit信号后，进程B收到，并且回复确认quit并退出，进程A收到确认quit后退出。
void *Listening_QUIT(void* arg)
{
    printf("into listening quit thread.\n");
    qids *qs = (qids*)arg;
    int sndqid = qs->qid1;
    int rcvqid = qs->qid2;
    msgmbuf rbuf;
    int rtype = QUITCODE;
    int msg_rflags = 0;
    _receive(rcvqid, &rbuf, rtype, msg_rflags);    // 阻塞监听QUITCODE
    if(rbuf.mtype == QUITCODE) {
        if(strcmp(rbuf.mtext, QUITMSG) == 0) {
            // 收到对方发来的QUIT请求信息，回复确认退出并结束当前进程。
            _send(sndqid, QUITCONFIRM, QUITCODE, 0);  // 阻塞发送确认消息
            // 通知主线程
            if (kill(getpid(), SIGKILL) == -1) {
                perror("kill");
                exit(EXIT_FAILURE);
            }

        } else if(strcmp(rbuf.mtext, QUITCONFIRM) == 0) {
            // 收到对方的确认退出，结束当前进程。
            // 通知主线程
            if (kill(getpid(), SIGKILL) == -1) {
                perror("kill");
                exit(EXIT_FAILURE);
            }

        }
    }
}


// 创建两个消息队列，id编号放入idA和idB中。
void get_msgqid(int *idA, int *idB)
{
    int ret = -1;
    char msgpathA[] = "./msg/qA";
    char msgpathB[] = "./msg/qB";
    key_t keyA = ftok(msgpathA, 'a');
    if(keyA != -1){
        printf("keyA created successfully.\n");
        printf("keyA:%#x\n\n", keyA);
    } else {
        printf("keyA fails to create.\n");
    }
    key_t keyB = ftok(msgpathB, 'b');
    if(keyB != -1){
        printf("keyB created successfully.\n");
        printf("keyB:%#x\n\n", keyB);
    } else {
        printf("keyB fails to create.\n");
    }

    int msgqflags = IPC_CREAT;
    *idA = msgget(keyA, msgqflags | 0666);
    printf("idA: %d\n", *idA);
    if(*idA == -1) {
        printf("msg queue A fails to establish.\n");
        exit(EXIT_FAILURE);
    }
    
    *idB = msgget(keyB, msgqflags | 0666);
    printf("idB: %d\n", *idB);
    if(*idB == -1) {
        printf("msg queue B fails to establish.\n");
        exit(-1);
    }
    printf("\n");
    return;
}

void clear_EXITmsg(int qid) {
    printf("Clear EXIT msg...\n");
    msgmbuf rbuf;
    int msg_rflags = IPC_NOWAIT | MSG_NOERROR;
    while (_receive(qid, &rbuf, QUITCODE, msg_rflags) != -1);
    return;
}


void start(int id1, int id2)    // id1:发送队列,id2:接收队列
{
    clear_EXITmsg(id2);

    // 创建另一个线程来监听quit消息
    pthread_t tid;
    qids qs = {id1, id2};
    int result = pthread_create(&tid, NULL, Listening_QUIT, &qs);
    if (result != 0) {
        perror("Listening_QUIT thread creation failed");
        exit(EXIT_FAILURE);
    }
    // BUG:为什么要先输入回车，才能开始另一个线程？


    char cmd;
    printf("(s for send, r for receive, w for msgqinfo)\nPlease input cmd: ");
    scanf("%c", &cmd);

    while(1) {
        switch (cmd)
        {
            case 's': {
                char msg[256];
                long stype;
                printf("Input msg: ");
                scanf("%s", msg);
                printf("msg type (int, not 8 9): ");
                scanf("%ld", &stype);

                int msgsflag = IPC_NOWAIT;   // 消息队列满时是否阻塞等待有足够的空间来存储消息。
                int ret = _send(id1, msg, stype, msgsflag); // 非阻塞发送
                if(ret == -1) {
                    printf("消息发送失败.\n");
                } else {
                    printf("msg:%s, type:%ld, len:%ld, 消息发送成功.\n", msg, stype, (size_t)strlen(msg));
                }

                // 等待确认
                printf("等待确认......\n");
                msgmbuf rbuf;
                memset(&rbuf, 0, (size_t)sizeof(rbuf));
                int rtype = CONFIRMCODE;
                int rflag = MSG_NOERROR;
                ret = _receive(id2, &rbuf, rtype, rflag);  // 阻塞接收
                if(ret == -1) {
                    printf("从队列 %d 中接收confirm消息失败.\n", id2);
                } else {
                    char confirm_num[256] = {0};
                    sprintf(confirm_num, "%ld", stype);
                    if(rbuf.mtype == CONFIRMCODE && strcmp(rbuf.mtext, confirm_num) == 0) printf("对方确认收到消息!\n");
                }

                break;
            }
            case 'r': {
                // 接收消息
                msgmbuf msg_rbuf;
                memset(&msg_rbuf, 0, (size_t)sizeof(msg_rbuf));
                long rtype;
                printf("Input expected msg type: ");
                scanf("%ld", &rtype);
    
                int msgrflag = IPC_NOWAIT | MSG_NOERROR;
                int ret = _receive(id2, &msg_rbuf, rtype, msgrflag);  // 非阻塞接收
                if(ret == -1) {
                    printf("队列 %d 中没有 %ld 类型的消息.\n", id2, rtype);
                } else {
                    printf("接收消息成功，长度: %d\n", ret);
                    // printf("type: %ld\n", msg_rbuf.mtype);
                    // printf("text: %s\n", msg_rbuf.mtext);
                    if(msg_rbuf.mtype == QUITCODE) {
                        exit(0);
                    } else if (strcmp(msg_rbuf.mtext, "") == 0) {
                        printf("暂无消息.\n");
                    } else {
                        printf("消息：%s\n", msg_rbuf.mtext);
                    }

                    // 回复确认
                    char confirm_num[256] = {0};
                    sprintf(confirm_num, "%ld", rtype);
                    // printf("confirm_num: %s\n", confirm_num);
                    ret = _send(id1, confirm_num, CONFIRMCODE, 0); // 阻塞发送
                    if(ret == -1) {
                        printf("confirm消息发送失败.\n");
                    } else {
                        printf("已自动确认.\n");
                    }
                }
                
                break;
            }
            case 'w': {
                struct msqid_ds msg_info;
                memset(&msg_info, 0, (size_t)sizeof(msg_info));
                printf("\nshow msgq 1 info: ");
                msg_show_attr(id1, msg_info);
                printf("\nshow msgq 2 info: ");
                msg_show_attr(id2, msg_info);
                break;
            }
            case 'q': {
                // 主动退出：主动发送一个QUIT信号，剩下的由线程处理。
                printf("quit......\n");
                _send(id1, QUITMSG, QUITCODE, 0);
                pause();
            }
        
            default:
                printf("Invalid input.\n");
                break;
        }
        printf("\ncmd: ");
        scanf(" %c", &cmd);
    }
}
