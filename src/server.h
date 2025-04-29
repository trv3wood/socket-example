class Server {
public:
    Server();
    ~Server();
    void start();
    void stop();
    void restart();
    void setPort(int port);
    int getPort() const;
    
};