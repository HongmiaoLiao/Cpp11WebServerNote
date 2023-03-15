# http/httpresponse.h、http/httpresponse.c

## 使用库

* sys/stat.h 头文件

```C++
#include <sys/stat.h>

// stat 结构体，用来描述一个linux系统文件系统中的文件属性的结构
struct stat {
    dev_t     st_dev;         /* ID of device containing file */
    ino_t     st_ino;         /* Inode number */
    mode_t    st_mode;        /* File type and mode */
    nlink_t   st_nlink;       /* Number of hard links */
    uid_t     st_uid;         /* User ID of owner */
    gid_t     st_gid;         /* Group ID of owner */
    dev_t     st_rdev;        /* Device ID (if special file) */
    off_t     st_size;        /* Total size, in bytes */
    blksize_t st_blksize;     /* Block size for filesystem I/O */
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */

    /* Since Linux 2.6, the kernel supports nanosecond
        precision for the following timestamp fields.
        For the details before Linux 2.6, see NOTES. */

    struct timespec st_atim;  /* Time of last access */
    struct timespec st_mtim;  /* Time of last modification */
    struct timespec st_ctim;  /* Time of last status change */

#define st_atime st_atim.tv_sec      /* Backward compatibility */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
};

// stat 系统调用函数，返回文件信息，将信息存入statbuf缓存
int stat(const char *restrict pathname,
        struct stat *restrict statbuf);

// munmap 系统调用函数，删除指定地址范围的映射
int munmap(void *addr, size_t length);
// 并导致对该范围内地址的进一步引用生成无效的内存引用。当流程终止时，区域也会自动取消映射。另一方面，关闭文件描述符并不会取消该区域的映射。

// 宏 S_ISDIR，根据stat结构中的st_mode 判断文件是否是一个目录
S_ISDIR(m)

// st_mode的S_IROTH掩码，表示读的权限
S_IROTH     00004   others have read permission
```

* fcntl.h 头文件

```C++
#include <fcntl.h>

// open 系统调用函数，打开由pathname指定的文件，返回文件描述符
int open(const char *pathname, int flags);
// 参数flags必须包括以下访问之一模式 O_RDONLY、O_WRONLY、O_RDWR。这些请求打开文件只读、只写或读/写。
```

* sys/mman.h 头文件

```C++
#include <sys/mman.h>

// mmap函数，在调用进程的虚拟地址空间中创建一个新的映射。新映射的起始地址在addr中指定。length参数指定映射的长度(必须大于0)。
void *mmap(void *addr, size_t length, int prot, int flags,
            int fd, off_t offset);
// prot为PROT_READ时，表示页面可能会被阅读
// flags为MAP_PRIVATE，创建私有的写时复制映射。对映射的更新对于映射同一文件的其他进程是不可见的，并且不会传递到底层文件。
// 如果成功，mmap()返回一个指向映射区域的指针。在错误时，返回值MAP_FAILED(即(void *) -1)， 并设置errno以指示错误。
```

## 笔记

构造函数：初始化一些私有成员变量

析构函数：调用UnmapFile取消文件在内存的映射

私有成员变量：

* code_：当前响应的状态码
* is_keep_alive_：标识是否为长连接
* path_：工作目录下的资源路径的字符串
* src_dir_：工作目录路径的字符串
* mm_file_：响应文件内容映射到内存的指针
* mm_file_stat_：响应读取的文件的信息
* kSuffixType：静态常量的字典，标识文件的[MIME类型](https://www.runoob.com/http/mime-types.html)
* kCodeStatus：静态常量的字典，标识状态码对应的状态信息
* kCodePath：静态常量的字典，标识错误码对应的错误页面的工作路径

私有成员函数：

AddStateLine_：为响应消息添加状态行

* 如果当前状态码再状态码字典中，设置响应状态为对应的信息，否则统一设置为400
* 然后拼接消息后，将响应消息状态行放进缓冲区，以\r\n结尾

AddHeader_：为响应消息添加两个消息报头，是否为长连接，响应文件类型

* 如果为长连接，超时时间为120m，最多接受6次请求就断开

AddContent_：添加响应体，如果出现错误就添加错误信息的响应题，否则只添加一个包含相应体长度信息的响应头字段

* 打开文件失败，则写入错误信息的页面，直接返回
* 将响应文件的内容映射到当前进程内存虚拟地址0开始的空间
  * 如果失败，mm_ret的值为-1，缓冲区写入错误信息的页面，直接返回
  * 如果映射成功，mm_ret指向的就是映射的地址，其内容为字符串（或者说以字符串方式读写），所以强转为字符串指针
* 为响应内容的长度添加响应头，有响应体的http的请求和响应才有这个内容，以\r\n\r\n结尾

ErrorHtml_：根据错误的状态码信息（如果code_时错误码），将路径指向标识错误的html页面

* 如果错误码在错误码页面字典中，获取错误码页面，并获取错误码页面的信息

GetFileType_：获取响应的文件的类型

* 如果没有后缀名，则认为是text文件，如果后缀名在文件类型字典中，则返回对应文件类型，否则也认为是text文件

公有成员函数：

Init：执行响应对象的初始化

* 如果已经存在内存映射，则取消映射
* 设置状态码、连接信息，路径信息，设置响应文件状态为空

MakeResponse：获取并拼接响应信息放入缓冲区

* 判断请求的资源文件
* 如果获取文件信息失败或者是一个目录，则code_设为404
* 如果不可读，code_设为403
* 获取文件成功且code_未赋值，则设置为200
* 如果有错误，将路径指向错误页面
* 将状态行，响应头，响应体（出错时错误信息的响应体）放到缓冲区

UnmapFile：删除响应文件在内存中的映射

File：返回响应文件映射到内存的起始地址指针，即返回mm_file_

FileLen：返回响应文件的大小

ErrorContent：添加错误的响应体信息，将message信息写入到一个html页面中后放入缓冲区

Code：返回状态码，即返回私有成员变量code
