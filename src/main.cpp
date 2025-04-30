#include "server.h"

int main() {
    Server server;
    server.port(8080).start().run();
}
