#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <termios.h>

// Настройки сервера
#define PORT 8080
#define BUFFER_SIZE 1024

// Настройки Serial (для Arduino)
#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE B9600

int serial_fd;

// Функция для открытия Serial-порта
int openSerialPort(const char* port, speed_t baud_rate) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Ошибка открытия Serial-порта");
        return -1;
    }
    
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, baud_rate);
    cfsetospeed(&options, baud_rate);
    
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    
    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

int main() {
    std::cout << "Запуск TCP-сервера для OmegaBot..." << std::endl;

    // 1. Открываем Serial-порт для Arduino
    serial_fd = openSerialPort(SERIAL_PORT, BAUD_RATE);
    if (serial_fd == -1) {
        std::cerr << "Не удалось открыть Serial-порт" << std::endl;
        return 1;
    }
    std::cout << "Serial-порт открыт: " << SERIAL_PORT << std::endl;

    // 2. Создаем TCP-сервер
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Создаем сокет
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Ошибка создания сокета");
        return 1;
    }

    // Настройка опций сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Ошибка настройки сокета");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Ошибка привязки сокета");
        return 1;
    }

    // Прослушивание порта
    if (listen(server_fd, 3) < 0) {
        perror("Ошибка прослушивания");
        return 1;
    }

    std::cout << "Сервер запущен. Ожидание подключения на порту " << PORT << "..." << std::endl;

    // 3. Принимаем подключение
    int client_fd;
    if ((client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Ошибка принятия подключения");
        return 1;
    }

    std::cout << "Клиент подключен!" << std::endl;

    // 4. Основной цикл обработки команд
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_fd, buffer, BUFFER_SIZE);
        
        if (valread <= 0) {
            std::cout << "Клиент отключился" << std::endl;
            break;
        }

        // Полученная команда
        char command = buffer[0];
        std::cout << "Получена команда: '" << command << "'" << std::endl;

        // Отправляем команду в Arduino
        if (write(serial_fd, &command, 1) == -1) {
            perror("Ошибка отправки в Serial");
        }

        // Эхо-ответ клиенту
        send(client_fd, buffer, valread, 0);
    }

    // Закрываем соединения
    close(client_fd);
    close(server_fd);
    close(serial_fd);
    
    std::cout << "Сервер остановлен" << std::endl;
    return 0;
}
