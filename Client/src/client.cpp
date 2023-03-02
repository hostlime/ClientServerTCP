
#include <iostream>
#include <global.hpp>
#include <locale>

using asio::ip::tcp;

int main(int argc, char* argv[])
{
    uint16_t port = DEFAULT_PORT;
    std::string host = DEFAULT_HOST;

//#ifndef _DEBUG
    //*******************��������� ����� ��� �������������**********************
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
//#endif
    //************************************************************************



    std::cout << "************** Start TCP CLIENT **************" << std::endl;
    // ��������� ������ ��� ����������� ����������� ������� ����
    setlocale(LC_ALL, "");
    // 
    TcpPackage::Request request;            // ��������� �������
    TcpPackage::Response response;          // ��������� ������
    std::vector<uint8_t> Buff(2048);        // ����� ��� ������ � ��������

        while (true) {
            {
                try {
                    // �������������� ���������� asio
                    asio::io_context io_context;

                    // ������� ������� ��� ���������� � �������� � �������� ������
                    tcp::socket socket(io_context);
                    tcp::resolver resolver(io_context);
                    asio::connect(socket, resolver.resolve(host, std::to_string(port)));
                    std::cout << "���������� � �������� "<< host << ":" << port <<" �����������!" << std::endl;
                    std::cout << "������� �������: 'c:/' ��� 'd:/music' " << std::endl;
                    while (true) {
                        Buff.clear();
                        Buff.resize(2048);
                        // ������ ���������������� ���� �� �������
                        std::cout << "������� ������: ";
                        std::string input;
                        std::getline(std::cin, input);

                        // ��������� ��������� �������
                        request.head.type = TcpPackage::GetFileList_TYPE;
                        request.head.len = static_cast<uint32_t>(input.size());
                        // ����������� ����� � ������

                        request.body.buff.resize(input.size());
                        std::memcpy(request.body.buff.data(), input.c_str(), input.size());
                        {
                            //��������� �� ����� ��� ��������� ������������
                            asio::mutable_buffer buffer(Buff.data(), Buff.capacity() + 4);
                            
                            // ��������� �� ����� ����� ����� ������� ����� ����
                            TcpPackage::Request* ptrHead = reinterpret_cast<TcpPackage::Request*>(buffer.data());
                            size_t lenHead, lenBody;

                            // ����������� ������
                            lenHead = request.head.serialize(buffer);
                            lenBody = request.body.serialize(buffer);
                            ptrHead->head.len = static_cast<uint32_t>(lenBody);
                            Buff.resize(lenHead + lenBody);

                            // ���������� ������ �� ������
                            asio::write(socket, asio::buffer(Buff.data(), Buff.size()));
                        }


                        // �������� ����� �� �������
                        size_t response_length = asio::read(socket, asio::buffer(&response, sizeof(response.head)));
                        std::cerr << "������� ����� ���� ������: " << response.head.len << std::endl;
                        
                        // ����������� ������������ ��� ������
                        Buff.resize(response.head.len);
                        response_length = asio::read(socket, asio::buffer(Buff));

                        // ������� ����� �� �����
                        std::cout << "����� �� �������: " << std::endl;

                        asio::mutable_buffer bufferFile(Buff.data(), Buff.size());
                        response.body.deserialize(bufferFile);

                        for (auto& elem : response.body.files) {
                            std::cout << "{" << elem.path << ", " << elem.type << ", " << elem.size << "}" << std::endl;
                        }

                        std::cout << std::endl;
                    }
                } catch (std::exception& e) {
                    std::cerr << "������ ����������� � " <<host<<":"<< port << std::endl <<"e.what()->" << e.what() << std::endl;
                    Sleep(1000);
                }
                
            }
        }


    return 0;
}