#include <cstdio>
#include <cstdlib>
void run_command(const char *command) {
    FILE *fp;
    char buffer[128];
    fp = popen(command, "r");
    if (fp == nullptr) {
        perror("popen");
        exit(EXIT_FAILURE);
    }
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        printf("%s", buffer);
    }
    if (pclose(fp) == -1) {
        perror("pclose");
        exit(EXIT_FAILURE);
    }
}
int main() {
    const char *command = "wrk -t4 -c100 -d20s http://localhost:8080/";
    run_command(command);
    return 0;
}