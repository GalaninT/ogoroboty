#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <wiringSerial.h>

class RobotServer {
private:
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int arduino_port;
    
public:
    RobotServer(int port) {
        // Создание TCP сокета
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        
        // Настройка опций сокета
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        // Привязка сокета к порту
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        
        // Подключение к Arduino
        arduino_port = serialOpen("/dev/ttyACM0", 9600);
        if (arduino_port < 0) {
            std::cerr << "Unable to open serial connection to Arduino" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        std::cout << "Connected to Arduino" << std::endl;
    }
    
    void start() {
        // Прослушивание входящих подключений
        if (listen(server_fd, 3) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }
        
        std::cout << "Server listening on port 8080..." << std::endl;
        
        while (true) {
            // Принятие входящего подключения
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            
            handleClient(new_socket);
            close(new_socket);
        }
    }
    
private:
    void handleClient(int client_socket) {
        char buffer[1024] = {0};
        int valread;
        
        while ((valread = read(client_socket, buffer, 1024)) > 0) {
            std::string command(buffer, valread);
            std::cout << "Received command: " << command << std::endl;
            
            // Обработка команд и отправка на Arduino
            processCommand(command);
            
            // Ответ клиенту
            std::string response = "Command executed: " + command + "\n";
            send(client_socket, response.c_str(), response.length(), 0);
            
            memset(buffer, 0, sizeof(buffer));
        }
    }
    
    void processCommand(const std::string& command) {
        char arduino_command = 'S'; // По умолчанию - стоп
        
        if (command == "FORWARD" || command == "F") arduino_command = 'F';
        else if (command == "BACKWARD" || command == "B") arduino_command = 'B';
        else if (command == "LEFT" || command == "L") arduino_command = 'L';
        else if (command == "RIGHT" || command == "R") arduino_command = 'R';
        else if (command == "STOP" || command == "S") arduino_command = 'S';
        
        // Отправка команды на Arduino
        serialPutchar(arduino_port, arduino_command);
        std::cout << "Sent to Arduino: " << arduino_command << std::endl;
    }
};

int main() {
    RobotServer server(8080);
    server.start();
    return 0;
}