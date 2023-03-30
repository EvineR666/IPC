
// 利用消息队列的基本函数，实现Linux系统下进程A和进程B之间的消息收发。
// 进程A可以发送消息给进程B，并且进程B可以确认消息；A发给B并且阻塞接收，B接收后自动回复A，A接收到之后确认收到。。
// 进程B可以发送消息给进程A，并且进程A可以确认消息；
// 当一方发出退出信息之后，双方进程退出（请认真思考）。

// 用两个终端运行两个进程，两个进程要能相互发消息，需要两个消息队列。
// while，每次都等待用户键入，scanf，如果是s进入发送函数，如果是r就进入接收函数。


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#define CONFIRMCODE 8
#define QUITCODE 9
#define QUITMSG "QUIT"
#define QUITCONFIRM "CONFIRM"

typedef struct msgmbuf{
        long mtype;
        char mtext[256];
    }msgmbuf;

typedef struct qids {
    int qid1;
    int qid2;
}qids;



