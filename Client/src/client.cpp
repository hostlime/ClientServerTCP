
#include <iostream>
#include <global.hpp>
#include <locale>

using asio::ip::tcp;

struct Request {
    std::uint32_t lenRequest;
    std::string path;
};

const int max_length = 10240;

int main(int argc, char* argv[]) {
    std::cout << "Start TCP CLIENT" << std::endl;
    // Установка локали для корректного отображения русских букв
    std::locale::global(std::locale("en_US.utf8"));

        while (true) {
            // Читаем пользовательский ввод из консоли
            std::cout << "Введите запрос: ";
            std::string input;
            std::getline(std::cin, input);

            // Формируем структуру запроса
            Request request;
            request.path = input;
            request.lenRequest = request.path.length();


            std::vector<asio::const_buffer> buffers;
            buffers.push_back(asio::buffer(&request.lenRequest, sizeof(request.lenRequest)));
            buffers.push_back(asio::buffer(request.path));
            {

                try {
                // Инициализируем библиотеку asio
                asio::io_context io_context;

                // Создаем объекты для соединения с сервером и отправки данных
                tcp::socket socket(io_context);
                tcp::resolver resolver(io_context);
                asio::connect(socket, resolver.resolve("localhost", "12346"));

                // Отправляем запрос на сервер
                asio::write(socket, buffers);

                // Получаем ответ от сервера
                char response[max_length];
                uint32_t lenBody;
                size_t response_length = asio::read(socket, asio::buffer(&lenBody, 4));
                std::cerr << "Ожидаем прием тела длиной: " << lenBody << std::endl;

                response_length = asio::read(socket, asio::buffer(response, lenBody));

                // Выводим ответ на экран
                std::cout << "Ответ от сервера: ";
                std::cout.write(response, lenBody);
                std::cout << std::endl;
                }
                catch (std::exception& e) {
                    std::cerr << "Ошибка: " << e.what() << std::endl;
                }
            }
        }


    return 0;
}