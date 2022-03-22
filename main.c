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
    int byte_count = 0;
    char *byte_to_write = "A";
    struct flock region_1;
    struct flock region_2;
    int res = 0;
 
    // 打开一个文件m描述符
    file_desc = open(test_file, O_RDWR|O_CREAT, 0666);
    if (!file_desc)
    {
        fprintf(stderr, "Unable to open %s for read/write\n", test_file);
        exit(EXIT_FAILURE);
    }
 
    // 给文件添加 100个 'A'字符的数据
    for (byte_count = 0; byte_count < 100; ++byte_count)
    {
        write(file_desc, byte_to_write, 1);
    }
    
    // 在文件的第 10~29 字节设置读锁(共享锁)
    region_1.l_type = F_RDLCK;
    region_1.l_whence = SEEK_SET;
    region_1.l_start = 10;
    region_1.l_len = 20;
    region_1.l_pid = -1;
 
    // 在文件的 40~49 字节设置写锁(独占锁)
    region_2.l_type = F_WRLCK;
    region_2.l_whence = SEEK_SET;
    region_2.l_start = 40;
    region_2.l_len = 10;
    region_2.l_pid = -1;
 
    printf("Process %d locking file\n", getpid());
 
    // 锁定文件
    res = fcntl(file_desc, F_SETLK, &region_1);
    if (res == -1)
    {
        fprintf(stderr, "Failed to lock region 1, error: %d: %s\n", errno,strerror(errno));
    } else
    {
        printf("Process %d locking file Region 1\n", getpid());
    }
 
    res = fcntl(file_desc, F_SETLK, &region_2);
    if (res == -1)
    {
        fprintf(stderr, "Failed to lock region 2, error: %d: %s\n", errno, strerror(errno));
    } else
    {
        printf("Process %d locking file Region 2\n", getpid());
    }
 
    // 让程序休眠1分钟, 用于测试
    sleep(60);
    printf("Process %d closing file\n", getpid());
    close(file_desc);
    exit(EXIT_SUCCESS);
}