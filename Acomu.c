
#include "comu.h"

int main()
{
    // 删除已存在的消息队列
    // system("ipcrm -a");

    // 创建/获取两个消息队列的ID
    int idA, idB;
    get_msgqid(&idA, &idB);
    
    int my_q = idA;
    int target_q = idB;

    // struct msqid_ds msg_info;
    // printf("show msgq A info:  ");
    // msg_show_attr(idA, msg_info);
    // printf("show msgq B info: ");
    // msg_show_attr(idB, msg_info);

    // 开始主循环, 参数：发送队列,接收队列
    start(target_q, my_q);
}