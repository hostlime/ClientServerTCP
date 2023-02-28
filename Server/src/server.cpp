// Your First C++ Program
#include <global.hpp>
std::array<char, 1024> serBuff; // создаем массив размером 1024 байта


struct FileDescriprion {
	//{<путь к файлу>, <тип файла>, <размер файла>}
	std::string path;
	std::string type;
	UINT32 size;

	// Бинарная сериализация
	static void serialize(FileDescriprion const& value, Serializer& s) {
		//Serializer s(buffer);
		s(value.path);
		s(value.type);
		s(value.size);
	}
	// Бинарная десериализация
	static void deserialize(FileDescriprion& value, Deserializer& d) {
		//Deserializer d(buffer);
		d(value.path);
		d(value.type);
		d(value.size);
	}
};



// Структура ЗАГОЛОВКА ответа
struct ResponseHead {
	uint32_t type;	// тип данных
	uint32_t len;	// длина ответа
	
public:
	size_t serialize(asio::mutable_buffer & buffer) {
		Serializer s(buffer);
		s(type);
		s(len);
		buffer += s.offset();
		return s.offset();
	}

	// Бинарная десериализация заголовка ответа
	size_t deserialize(asio::mutable_buffer& buffer) {
		Deserializer d(buffer);
		d(type);
		d(len);
		buffer += d.offset();
		return d.offset();
	}
};
// Структура ТЕЛА ответа
struct ResponseBody {
	std::vector<FileDescriprion> files;
	
	// Бинарная сериализация  тела ответа
	std::size_t serialize(asio::mutable_buffer & buffer) {
		Serializer s(buffer);
		uint32_t len = static_cast<uint32_t>(files.size());
		s(len);
		for (auto const& elem : files) {
			elem.serialize(elem, s);
		}
		buffer += s.offset();
		return s.offset();
	}
	void deserialize(asio::mutable_buffer & buffer) {
		Deserializer d(buffer);
		uint32_t len;
		d(len); // считываем количество файлов
		files.resize(len);
		for (auto & elem : files) {
			elem.deserialize(elem, d);
		}
		buffer += d.offset();
		// return d.offset(); затруднительно посчитать реальную длину на выходе
	}
};
struct RequestBody {
	std::vector<uint8_t> buff;

	// Бинарная сериализация ЗАПРОСА
	std::size_t serialize(asio::mutable_buffer& buffer) {
		Serializer s(buffer);
		s(buff);
		buffer += s.offset();
		return s.offset();
	}

	// Бинарная сериализация ЗАПРОСА
	std::size_t deserialize(asio::mutable_buffer& buffer) {
		Deserializer d(buffer);
		d(buff);

		buffer += d.offset();
		return buff.size();	// Почему не d.offset() ? 
		// дело в том что при десереализации количество данных на выходе не равно offset()
		// offset() учитывает и количество байт затраченых на длины типов данных
	}
};

// Структура ЗАПРОСА
struct Request {
	ResponseHead head;
	RequestBody body;
};
// Структура ОТВЕТА
struct Response {
	ResponseHead head;
	ResponseBody body;
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


// Методы для работы с файлами
class FileManager {
public:
	// Запись списка файлов в вектор по указанному пути
	void WriteFileList(std::vector<FileDescriprion>& files, const fs::path& directory_path) {
		files.clear();
		if (fs::is_directory(directory_path))
			for (const auto& entry : fs::directory_iterator(directory_path)) {
				//{<путь к файлу>, <тип файла>, <размер файла>}
				files.push_back({
					entry.path().string(),
					get_file_type(entry),
					static_cast<uint32_t>(fs::file_size(entry.path())),
					});
#ifdef _DEBUG
				// Выводим список для отладки
				const fs::path& file_path = entry.path();
				const std::string& file_type = get_file_type(entry);
				const uintmax_t file_size = fs::file_size(file_path);
				std::cout << "{" << file_path << ", " << file_type << ", " << file_size << "}" << std::endl;
#endif
			}

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
};


// Класс ответа
class TcpPackage :FileManager {
public:
	void * requestHeadData() {
		return &request.head;
	}
	size_t requestHeadSizeof() {
		return sizeof(request.head);
	}

	std::vector<uint8_t>* requestBodyData() {
		// Тут можно было бы десерелиазовать заголовок для получения длины тела
		// но в этом нет смысла т.к сейчас заголовок состоит из простых типов данных
		// и сериализация его не меняет

		request.body.buff.resize(request.head.len);
		return &request.body.buff;
	}
	void makeResponse(std::shared_ptr<std::vector<uint8_t>> DATA)
	{
		switch (request.head.type) {
		case 1:
			size_t lenHead, lenBody;
			//************************десериализуем тело запроса***************************
			{
				DATA->resize(1024); 
				asio::mutable_buffer buffer(request.body.buff.data(), request.body.buff.size());// сериализация пока только с mutable_buffer(для скорости)
				lenBody = request.body.deserialize(buffer);
				// Преобразование array в строку
				std::string spath(request.body.buff.begin(), request.body.buff.end());
				fs::path file_path = fs::path(spath);
				// Заполняем информацию о файлах по пути file_path
				WriteFileList(response.body.files, file_path);
				DATA->resize(response.body.files.size() * sizeof(response.body.files) + 1024); // на всякий слачай резирвируем больше данных чтобы десереализованные данные не вышли за границу

			}
			// Сериализуем данные для дальнейшей отправки
			asio::mutable_buffer buffer(DATA->data(), DATA->size());
			Response* ptrResponse = reinterpret_cast<Response*>(buffer.data());

			lenHead = response.head.serialize(buffer);
			lenBody = response.body.serialize(buffer);

			ptrResponse->head.type = request.head.type; //копируем тип
			ptrResponse->head.len = lenBody; // записываем длину пакета
			DATA->resize(lenHead + lenBody);
			break;
		}
		
	}
	TcpPackage() {
		request.body.buff.reserve(DEFAULT_BUFFSIZE_FOR_TCP_PACKAGE);		// Резервируем для сырых данных принятых по сети
	}
private:
	Request request;		// структура запроса
	Response response;		// структура ответа
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
		auto tcpPackage = std::make_shared<TcpPackage>();
		asio::async_read(socket_, asio::buffer(tcpPackage->requestHeadData(), tcpPackage->requestHeadSizeof()),
			[this, self, tcpPackage](std::error_code ec, std::size_t len) {
				if (!ec) {
					doReadBody(tcpPackage);
				}
				else {
					// обработка ошибки
					std::cerr << "Error while reading header: " << ec.message() << std::endl;
				}
			});
	}
	// Чтение тела запроса
	void doReadBody(std::shared_ptr<TcpPackage> tcpPackage) {
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
				}
				else {
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
	std::cout << "************** Start TCP SERVER **************" << std::endl;

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
