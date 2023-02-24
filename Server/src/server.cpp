// Your First C++ Program
#include <global.hpp>



struct FileDescriprion {
	//{<путь к файлу>, <тип файла>, <размер файла>}
	std::string path;
	std::string type;
	UINT32 size;

	// Функция сериализации структуры FileDescription
	template<typename Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& path;
		ar& type;
		ar& size;
	}
};

// Структура ЗАПРОСА
struct Request {
	std::uint32_t lenRequest;   // Длина тела запроса
	std::string path;           // тело запроса
};


// Структура ЗАГОЛОВКА ответа
struct ResponseHead {
	uint8_t type;	// тип данных
	uint16_t len;	// длина ответа
	
	template<typename Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& type;
		ar& len;
	}
};
// Структура ТЕЛА ответа
struct ResponseBody {
	std::vector<FileDescriprion> files;

	// Функция сериализации тела ответа
	template<typename Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& files;
	}
};

namespace fs = std::filesystem;

// методы для работы с кэшем
struct cache {
	bool is_cache(const fs::path& directory_path) {
		// .........
		return false;
	}
	template<typename T1, typename T2>
	void setFromCache(T1, T2) {
	// .........
	}
};

// Класс ответа
class tcpMessage : cache {

public:
	// Конструктор для сервера(С помощью пути заполняем данными о списке файлов)
	tcpMessage(const fs::path& directory_path) {
		body.files.clear();
		setData(directory_path);
	}
	// Конструктор для клиента(чтобы из сериализованных даных получить всю информацию)
	tcpMessage(const std::string& stringstream) {
		std::istringstream ioarchive_header(stringstream);
		boost::archive::binary_iarchive ioarchive(ioarchive_header);
		ioarchive >> head;
		ioarchive >> body;
	}

	const std::string getSerilizeData() {
		std::ostringstream archive_stream;
		boost::archive::binary_oarchive archive(archive_stream);
		archive << head << body;
		return archive_stream.str();
	}
private:
	// метод для получения типа данных файла
	std::string get_file_type(const fs::directory_entry& entry) {
		if (entry.is_directory()) {
			return "directory";
		}
		else if (entry.is_regular_file()) {
			return "file";
		}
		else {
			return "unknown";
		}
	}
	void setData(const fs::path& directory_path) {
		// Проверяем в cache
		if (is_cache(directory_path)) {
			setFromCache(body.files, directory_path);
		}
		else {
			if (fs::is_directory(directory_path))
				for (const auto& entry : fs::directory_iterator(directory_path)) {
					//{<путь к файлу>, <тип файла>, <размер файла>}
					body.files.push_back({
						entry.path().string(),
						get_file_type(entry),
						static_cast<uint32_t>(fs::file_size(entry.path())),
						});
#ifdef _DEBUG
					const fs::path& file_path = entry.path();
					const std::string& file_type = get_file_type(entry);
					const uintmax_t file_size = fs::file_size(file_path);
					std::cout << "{" << file_path << ", " << file_type << ", " << file_size << "}" << std::endl;
#endif
				}
		}
	}

	ResponseBody body;
	ResponseHead head;
};





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
		asio::async_read(socket_, asio::buffer(&request_.lenRequest, sizeof(request_.lenRequest)),
			[this, self](std::error_code ec, std::size_t len) {
				if (!ec) {
					request_.path.clear();
					request_.path.resize(request_.lenRequest);
					doReadBody();
				}
				else {
					// обработка ошибки
					std::cerr << "Error while reading header: " << ec.message() << std::endl;
				}
			});
	}
	// Чтение тела запроса
	void doReadBody() {
		auto self = shared_from_this();
		asio::async_read(socket_, asio::buffer(request_.path),
			[this, self](std::error_code ec, std::size_t len) {
				if (!ec) {
					std::cout << "Recieved message: " << request_.path.data() << std::endl;

					//**************************ФОРМИРУЕМ ДАННЫЕ ДЛЯ ОТВЕТА****************************
					tcpMessage msg(fs::path(request_.path));
					std::string serMsg = msg.getSerilizeData();

					// Создаем вектор в куче чтобы сделать на него умный указатель 
					// и передавать асинхронно по сети
					std::vector<asio::const_buffer>* vecPtrMsg = new std::vector<asio::const_buffer>();
					std::shared_ptr<std::vector<asio::const_buffer>> vecSharedPtrMsg(vecPtrMsg);

					// Сначала записываем длину заголовка а потом сериализованные данные
					uint32_t size = static_cast<std::uint32_t>(serMsg.size());

					// чтобы скопировать uint32_t в вектор приходится извращаться.....
					vecSharedPtrMsg->resize(1); // устанавливаем размер вектора на 1 элемент
					std::memcpy(vecSharedPtrMsg->data(), &size, sizeof(size)); // копируем значение size в начало буфера вектора

					vecSharedPtrMsg->push_back(asio::buffer(serMsg));
					//**********************************************************************************

					doWrite(vecSharedPtrMsg); // отправляем ответ
				}
				else {
					// обработка ошибки
					std::cerr << "Error while reading body: " << ec.message() << std::endl;
				}
			});
	}
	// отправка данных
	void doWrite(std::shared_ptr<std::vector<asio::const_buffer>>& message)
	{
		auto self = shared_from_this();
		asio::async_write(socket_, asio::buffer(*message, message->size()),
			[this, self, message](std::error_code ec, std::size_t lenght)
			{
				if (!ec)
				{
					std::cout << "Sent message: " << message->size() << std::endl;
					doReadHeader();
				}
				else {
					// обработка ошибки
					std::cerr << "Error while writing response: " << ec.message() << std::endl;
				}
			});
	}
	asio::ip::tcp::socket socket_;
	Request request_;
};

class Server
{
public:
	Server(asio::io_context& io_context, short port) :
		io_context_(io_context),
		acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{
		doAccept();
	}

private:
	void doAccept()
	{
		auto socket = std::make_shared<asio::ip::tcp::socket>(io_context_);
		acceptor_.async_accept(*socket, [this, socket](const std::error_code& error) {
			if (!error) {
				auto new_session = std::make_shared<Session>(std::move(*socket));
				new_session->start();
			}
			else {
				// обработка ошибки
				std::cerr << "Error while accepting new connection: " << error.message() << std::endl;
			}
		doAccept(); });
	}
	asio::ip::tcp::acceptor acceptor_;
	asio::io_context& io_context_;
};




int main()
{
	std::cout << "Start TCP SERVER" << std::endl;

	asio::io_context io_context;
	Server server(io_context, 12346);

	std::vector<std::thread> threats;
	for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
	{
		threats.emplace_back([
			&io_context, i]() {
				std::string str = "Start thread N-" + std::to_string(i);
			std::cout << str << std::endl;
			io_context.run();
			});
	}

	// Ожидаем завершения работы потоков
	for (auto& thread : threats)
	{
		thread.join();
	}
	return 0;
}
