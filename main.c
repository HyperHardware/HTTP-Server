#include <stdio.h>
#include <WinSock2.h>
#include <sys/stat.h>
#include <stdbool.h>
#pragma comment(lib, "ws2_32.lib")
char rootpath[128] = "./html"; // 网站根目录
// 开启socket库
void initSocket() {
    // 确定socket版本信息
    WSADATA wsadata;
    if (WSAEINVAL == WSAStartup(MAKEWORD(2, 2), &wsadata) ){
        printf("WSAStartip failed : %d\n", WSAGetLastError());
        exit(-1);
    }

}

// 关闭socket库
void closehttpSocket() {
    WSACleanup();
}
// 开启服务器
SOCKET startServer(const char* ip, unsigned short port) {
    // 1.创建套接字
    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET) {
        printf("socket create failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }
    // 2.绑定ip地址和端口号
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // 大小端问题！
    addr.sin_addr.s_addr = inet_addr(ip);

    if (SOCKET_ERROR == bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        printf("bind failed: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }
    // 3.监听
    listen(fd, 5);

    return fd;
}

void sendFile(SOCKET fd, const char* filename);

// 处理客户端的请求
void requestHanding(SOCKET fd) {
    char buf[1024] = { 0 };
    // 接受一下客户端发送请求
    if (0 >= recv(fd, buf, 1024, 0)) {
        printf("recv failed: %d\n", WSAGetLastError());
        return;
    }
    // puts(buf);
    // 请求行 FET / HTTP/1.1
    char mothed[10] = { 0 };    // 请求方法
    char url[128] = { 0 };      // 请求资源
    char urlbackup[128] = { 0 };
    // 解析
    int index = 0;
    char* p = NULL;
    for (p = buf; *p != ' '; p++) {
        mothed[index++] = *p;
    }
    p++; //跳过空格
    index = 0;
    for (; *p != ' '; p++) {
        url[index++] = *p;
    }
    strcpy(urlbackup, url);
    // 根据不同的请求方法，进行处理
    if (strcmp(mothed, "GET") == 0) {
        // 如果请求的是 / ,发送index.html
        sprintf(url, "%s%s", rootpath, (strcmp(urlbackup, "/") == 0 ? "/index.html" : urlbackup));
        // 判断文件是否存在
        struct _stat32 stat;
        if (-1 == _stat32(url, &stat)) {
            // 文件不存在就发送404
            char file[128] = { 0 };
            sprintf(file, "%s%s", rootpath, "/404.html");
            printf("发送文件: %s\n", file);
            sendFile(fd, file);
        }
        else {
            // 文件存在就发送存在
            printf("发送文件： %s\n", url);
            sendFile(fd, url);
        }

    }
    else if (strcmp(mothed, "POST") == 0) {
        printf("POST mothed\n");
    }
}
// 获取文件类型
const char* contentType(const char* filename) {
    // 获取文件后缀
    char* p = strrchr(filename, '.');
    if (!p) {
        printf("没有后缀");
        return "application/octet-stream";
    }
    p++;
    printf("后缀类型：%s\n", p);
    if (strcmp(p, "html") == 0) {
        return "text/html";
    }
    else if (strcmp(p, "css") == 0) {
        return "text/css";
    }
    else if (strcmp(p, "png") == 0) {
        return "image/png";
    }
    else if (strcmp(p, "jpg") == 0) {
        return "image/jpeg";
    }
    return "text/html";
}

// 发送文件
void sendFile(SOCKET fd, const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        // 发送404响应
        const char* not_found_header =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n";
        send(fd, not_found_header, strlen(not_found_header), 0);
        FILE* f404 = fopen("./html/404.html", "rb");
        if (f404) {
            char buf[1024];
            while (!feof(f404)) {
                int len = fread(buf, 1, 1024, f404);
                send(fd, buf, len, 0);
            }
            fclose(f404);
        }
        closesocket(fd);
        return;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);

    // 构建并发送HTTP头部
    char header[512];
    int header_len = sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        contentType(filename),
        size
    );
    send(fd, header, header_len, 0);

    
    // 发送头部数据
    char buf[BUFSIZ] = { 0 };
    // 读取并发送
    // char buf[1024] = { 0 };
    while (!feof(fp)) {
        int len = fread(buf, sizeof(char), BUFSIZ, fp);
        send(fd, buf, len, 0);
    }
    
    fclose(fp);
    closesocket(fd);
}
int main()
{
    initSocket();
    SOCKET serfd = startServer("127.0.0.1", 80);
    // 4.获取客户端的描述符
    int count = 0;
    while (true) {
        if (count < 100000) {
            count++;
        }
        struct sockaddr_in addr;
        int addr_len = sizeof(addr);
        SOCKET client = accept(serfd, (struct sockaddr*)&addr, &addr_len);
        
        requestHanding(client);
        // char buf[] = "<h1>Hello world!</h1>";
        // send(client, buf, strlen(buf), 0);
        printf("socket accept次数: %d\n", count);

    }
    closesocket(serfd);
    closehttpSocket();
}

