#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <mutex>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;
const int MAX_THREADS = 10;  // 最大线程数限制

// 全局互斥锁，用于保护共享资源
std::mutex mtx;

// 客户端信息结构体
struct ClientInfo {
    int socket;
    sockaddr_in address;
};

// 线程函数，处理客户端连接
void handleClient(ClientInfo* client) {
    char buffer[BUFFER_SIZE] = {0};
    char client_ip[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "线程 " << std::this_thread::get_id() 
                  << " 处理客户端: " << client_ip << std::endl;
    }

    while (true) {
        // 读取客户端数据
        int bytes_read = read(client->socket, buffer, BUFFER_SIZE - 1);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                std::lock_guard<std::mutex> lock(mtx);
                std::cout << "客户端 " << client_ip << " 断开连接" << std::endl;
            } else {
                std::lock_guard<std::mutex> lock(mtx);
                std::cerr << "读取数据错误" << std::endl;
            }
            break;
        }

        buffer[bytes_read] = '\0';
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "从 " << client_ip << " 收到: " << buffer << std::endl;
        }

        // 将数据原样返回给客户端
        if (send(client->socket, buffer, bytes_read, 0) < 0) {
            std::lock_guard<std::mutex> lock(mtx);
            std::cerr << "发送数据失败" << std::endl;
            break;
        }
    }

    // 关闭客户端socket
    close(client->socket);
    delete client;
}

int main() {
    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "无法创建socket" << std::endl;
        return 1;
    }

    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "设置socket选项失败" << std::endl;
        return 1;
    }

    // 绑定地址和端口
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "绑定失败" << std::endl;
        return 1;
    }

    // 开始监听
    if (listen(server_fd, 5) < 0) {
        std::cerr << "监听失败" << std::endl;
        return 1;
    }

    std::cout << "服务器正在监听端口 " << PORT << "..." << std::endl;

    // 存储线程对象
    std::vector<std::thread> threads;

    while (true) {
        // 接受客户端连接
        ClientInfo* client = new ClientInfo;
        socklen_t addr_len = sizeof(client->address);
        client->socket = accept(server_fd, (struct sockaddr*)&client->address, &addr_len);
        
        if (client->socket < 0) {
            std::cerr << "接受连接失败" << std::endl;
            delete client;
            continue;
        }

        // 检查当前线程数
        {
            std::lock_guard<std::mutex> lock(mtx);
            // 清理已完成的线程
            for (auto it = threads.begin(); it != threads.end(); ) {
                if (it->joinable()) {
                    it->join();
                    it = threads.erase(it);
                } else {
                    ++it;
                }
            }

            // 如果线程数达到上限，等待
            while (threads.size() >= MAX_THREADS) {
                std::cout << "达到最大线程数(" << MAX_THREADS << ")，等待..." << std::endl;
                // 可以在这里添加更复杂的等待逻辑或拒绝新连接
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                // 再次尝试清理线程
                for (auto it = threads.begin(); it != threads.end(); ) {
                    if (it->joinable()) {
                        it->join();
                        it = threads.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        // 创建线程处理客户端
        try {
            threads.emplace_back(handleClient, client);
            
            // 新线程与主线程分离
            threads.back().detach();
            
            {
                std::lock_guard<std::mutex> lock(mtx);
                std::cout << "当前活跃线程数: " << threads.size() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "创建线程失败: " << e.what() << std::endl;
            close(client->socket);
            delete client;
        }
    }

    // 等待所有线程完成(实际上这里的代码不会执行)
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // 关闭服务器socket
    close(server_fd);
    return 0;
}
