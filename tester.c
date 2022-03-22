#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
 
int main(int argc, char **argv)
{
    const char *test_file = "./test_lock.txt";
    int file_desc = -1;
    int res = 0;
 
    // 打开数据文件
    file_desc = open(test_file, O_RDWR|O_CREAT, 0666);
    if (!file_desc)
    {
        fprintf(stderr, "Unable to open %s for read/write", test_file);
        exit(EXIT_FAILURE);
    }
 
    // 设置区域1的锁类型
    struct flock region_test1;
    region_test1.l_type = F_RDLCK;
    region_test1.l_whence = SEEK_SET;
    region_test1.l_start = 10;
    region_test1.l_len = 20;
    region_test1.l_pid = -1;
 
    // 设置区域2的锁类型
    struct flock region_test2;
    region_test2.l_type = F_RDLCK;
    region_test2.l_whence = SEEK_SET;
    region_test2.l_start = 40;
    region_test2.l_len = 10;
    region_test2.l_pid = -1;
 
    // 对区域1的是否可以加一个读锁进行测试
    res = fcntl(file_desc, F_GETLK, region_test1);
    if (res == -1)
    {
        fprintf(stderr, "Failed to get RDLCK, error %d: %s\n", errno, strerror(errno));
    } else if (region_test1.l_pid == -1)
    {
        // 可以加一个读锁
        printf("test: Process %d could lock\n", getpid());
    }
    else
    {
        // 不允许加一个读锁
        printf("test:Process %d get lock failure\n", getpid());
    }
 
    // 对q区域2是否可以加一个读锁进行测试
    res = fcntl(file_desc, F_GETLK, region_test2);
    if (res == -1)
    {
        fprintf(stderr, "Failed to get RDLCK, error %d: %s\n", errno, strerror(errno));
    } else if (region_test2.l_pid == -1)
    {
        // 可以加一个读锁
        printf("test: Process %d could lock\n", getpid());
    }
    else
    {
        // 不允许加一个锁
        printf("test:Process %d get lock failure\n", getpid());
    }
    close(file_desc);
    exit(EXIT_SUCCESS);
}