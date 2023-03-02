// Your First C++ Program
#include <thread>

#include <global.hpp>


class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(asio::ip::tcp::socket socket) : socket_(std::move(socket)) {}
	asio::ip::tcp::socket& socket() { return socket_; }
	void start()
	{
		std::cout << "New connection, socket descriptor: " << socket_.native_handle() << std::endl;
		doReadHeader();
	}

private:
	// чтение заголовка запроса
	void doReadHeader() {
		auto self = shared_from_this();
		auto tcpPackage = std::make_shared<TcpPackage::Msg>();
		asio::async_read(socket_, asio::buffer(tcpPackage->requestHeadData(), tcpPackage->requestHeadSizeof()),
			[this, self, tcpPackage](std::error_code ec, std::size_t len) {
				if (!ec) {
					// Есть данные для приема тела ?
					if (tcpPackage->requestBodyData()->size()) {
						doReadBody(tcpPackage);
					} else doReadHeader();
				} else {
					// обработка ошибки
					std::cerr << "Error while reading header: " << ec.message() << std::endl;
				}
			});
	}
	// Чтение тела запроса
	void doReadBody(std::shared_ptr<TcpPackage::Msg> tcpPackage) {
		auto self = shared_from_this();
		asio::async_read(socket_, asio::buffer(tcpPackage->requestBodyData()->data(), tcpPackage->requestBodyData()->size()),
			[this, self, tcpPackage](std::error_code ec, std::size_t len) {
				if (!ec) {
					// Выводим ответ на экран
					std::cout << "Recieved message: ";
					std::cout.write((char *)tcpPackage->requestBodyData()->data(), tcpPackage->requestBodyData()->size());
					std::cout << std::endl;
					//**************************ФОРМИРУЕМ ДАННЫЕ ДЛЯ ОТВЕТА***************************
					auto package = std::make_shared<std::vector<uint8_t>>();
					tcpPackage->makeResponse(package);
					doWrite(package);
					//**********************************************************************************
				} else {
					// обработка ошибки
					std::cerr << "Error while reading body: " << ec.message() << std::endl;
				}
			});
	}
	// отправка данных
	void doWrite(std::shared_ptr<std::vector<uint8_t>>& message)
	{
		auto self = shared_from_this();
		asio::async_write(socket_, asio::buffer(*message, message->size()),
			[this, self, message](std::error_code ec, std::size_t lenght) {
				if (!ec){
					std::cout << "Sent message: " << message->size() << std::endl;
					doReadHeader();
				} else {
					// обработка ошибки
					std::cerr << "Error while writing response: " << ec.message() << std::endl;
				}
			});
	}
	asio::ip::tcp::socket socket_;
};

class Server{
public:
	Server(asio::io_context& io_context, uint16_t port) :
		io_context_(io_context),
		acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)){
		std::cout << "The TCP server is listening on port " << port << std::endl;
		doAccept();
	}

private:
	void doAccept(){
		auto socket = std::make_shared<asio::ip::tcp::socket>(io_context_);
		acceptor_.async_accept(*socket, [this, socket](const std::error_code& error) {
			if (!error) {
				auto new_session = std::make_shared<Session>(std::move(*socket));
				new_session->start();
			} else {
				// обработка ошибки
				std::cerr << "Error while accepting new connection: " << error.message() << std::endl;
			}
		doAccept(); });
	}
	asio::ip::tcp::acceptor acceptor_;
	asio::io_context& io_context_;
};



int main(int argc, char* argv[]){
	//*******************ПОЛУЧЕНИЕ ПОРТА ДЛЯ ПРОСЛУШИВАНИЯ**********************
	uint16_t port = DEFAULT_PORT;
#ifndef _DEBUG
	// в режиме дебага выключаем
	if (argc > 1) { // Переданы аргументы?
		std::istringstream iss(argv[1]);
		if (iss >> port) {
			std::cout << "Using port " << port << " from command line argument." << std::endl;
		}
		else {
			std::cout << "Invalid port number provided. Using default port " << DEFAULT_PORT << "." << std::endl;
			port = DEFAULT_PORT;
		}
	}
	else {
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
#endif
	//************************************************************************

	std::cout << "************** Start TCP SERVER **************" << std::endl;
	asio::io_context io_context;
	Server server(io_context, port);

	std::vector<std::thread> threats;
	for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i){
		threats.emplace_back([
			&io_context, i]() {
			io_context.run();
			});
	}
	std::cout << "Start " << std::thread::hardware_concurrency()  << " thread" << std::endl;
	// Ожидаем завершения работы потоков
	for (auto& thread : threats){
		thread.join();
	}
	return 0;
}
