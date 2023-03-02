
#include <iostream>

#include <global.hpp>



using asio::ip::tcp;

int main(int argc, char* argv[])
{
    uint16_t port = DEFAULT_PORT;
    std::string host = DEFAULT_HOST;

#ifndef _DEBUG
    //*******************ПОЛУЧЕНИЕ ПОРТА ДЛЯ ПРОСЛУШИВАНИЯ**********************
    if (argc > 1) {
        std::istringstream iss(argv[1]);
        if (iss >> port) {
            std::cout << "Using port " << port << " from command line argument." << std::endl;
        }
        else {
            std::cout << "Invalid port number provided. Using default port " << DEFAULT_PORT << "." << std::endl;
            port = DEFAULT_PORT;
        }
    }

    if (argc > 2) {
        host = argv[2];
        std::cout << "Using host " << host << " from command line argument." << std::endl;
    }

    if (argc <= 1) {
        std::cout << "Enter a port number or press enter to use the default port (" << DEFAULT_PORT << "): ";
        std::string input;
        std::getline(std::cin, input);

        if (!input.empty()) {
            std::istringstream iss(input);
            if (iss >> port) {
                std::cout << "Using port " << port << "." << std::endl;
            }
            else {
                std::cout << "Invalid port number provided. Using default port " << DEFAULT_PORT << "." << std::endl;
                port = DEFAULT_PORT;
            }
        }
    }

    if (argc <= 2) {
        std::cout << "Enter a host or press enter to use the default host (" << DEFAULT_HOST << "): ";
        std::string input;
        std::getline(std::cin, input);

        if (!input.empty()) {
            host = input;
            std::cout << "Using host " << host << "." << std::endl;
        }
    }
#endif
    //************************************************************************



    std::cout << "************** Start TCP CLIENT **************" << std::endl;
    // Установка локали для корректного отображения русских букв
    setlocale(LC_ALL, "");
    // 
    TcpPackage::Request request;            // структура запроса
    TcpPackage::Response response;          // структура ответа
    std::vector<uint8_t> Buff(2048);        // буфер для приема и передачи

        while (true) {
            {
                try {
                    // Инициализируем библиотеку asio
                    asio::io_context io_context;

                    // Создаем объекты для соединения с сервером и отправки данных
                    tcp::socket socket(io_context);
                    tcp::resolver resolver(io_context);
                    asio::connect(socket, resolver.resolve(host, std::to_string(port)));
                    std::cout << "Connection to server " << host << ":" << port << " established!" << std::endl;
                    std::cout << "Request examples: 'c:/' or 'd:/music' " << std::endl;
                    while (true) {
                        Buff.clear();
                        Buff.resize(2048);
                        // Читаем пользовательский ввод из консоли
                        std::cout << "Enter query: ";
                        std::string input;
                        std::getline(std::cin, input);

                        // Формируем структуру запроса
                        request.head.type = TcpPackage::GetFileList_TYPE;
                        request.head.len = static_cast<uint32_t>(input.size());
                        // резервируем место в буфере

                        request.body.buff.resize(input.size());
                        std::memcpy(request.body.buff.data(), input.c_str(), input.size());
                        {
                            //Указатель на буфер для ускорения сериализации
                            asio::mutable_buffer buffer(Buff.data(), Buff.capacity() + 4);
                            
                            // указатель на ачало чтобы потом вписать длину тела
                            TcpPackage::Request* ptrHead = reinterpret_cast<TcpPackage::Request*>(buffer.data());
                            size_t lenHead, lenBody;

                            // сериализуем запрос
                            lenHead = request.head.serialize(buffer);
                            lenBody = request.body.serialize(buffer);
                            ptrHead->head.len = static_cast<uint32_t>(lenBody);
                            Buff.resize(lenHead + lenBody);

                            // Отправляем запрос на сервер
                            asio::write(socket, asio::buffer(Buff.data(), Buff.size()));
                        }


                        // Получаем ответ от сервера
                        size_t response_length = asio::read(socket, asio::buffer(&response, sizeof(response.head)));
                        std::cerr << "Expect a body length: " << response.head.len << std::endl;
                        
                        // Резервируем пространство под данные
                        Buff.resize(response.head.len);
                        response_length = asio::read(socket, asio::buffer(Buff));

                        // Выводим ответ на экран
                        std::cout << "Response from server: " << std::endl;

                        asio::mutable_buffer bufferFile(Buff.data(), Buff.size());
                        response.body.deserialize(bufferFile);

                        for (auto& elem : response.body.files) {
                            std::cout << "{" << elem.path << ", " << elem.type << ", " << elem.size << "}" << std::endl;
                        }

                        std::cout << std::endl;
                    }
                } catch (std::exception& e) {
                    std::cerr << "Error connecting to " << host << ":" << port << std::endl << "e.what()->" << e.what() << std::endl;
                    Sleep(1000);
                }
                
            }
        }


    return 0;
}