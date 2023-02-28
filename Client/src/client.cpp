
#include <iostream>
#include <global.hpp>
#include <locale>

using asio::ip::tcp;

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
	size_t serialize(asio::mutable_buffer& buffer) {
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
	std::size_t serialize(asio::mutable_buffer& buffer) {
		Serializer s(buffer);
		uint32_t len = static_cast<uint32_t>(files.size());
		s(len);
		for (auto const& elem : files) {
			elem.serialize(elem, s);
		}
		buffer += s.offset();
		return s.offset();
	}
	void deserialize(asio::mutable_buffer& buffer) {
		Deserializer d(buffer);
		uint32_t len;
		d(len); // считываем количество файлов
		files.resize(len);
		for (auto& elem : files) {
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

const int max_length = 10240;

int main(int argc, char* argv[]) {
    std::cout << "Start TCP CLIENT" << std::endl;
    // Установка локали для корректного отображения русских букв
    std::locale::global(std::locale("en_US.utf8"));

        while (true) {
            Request request;
            // Читаем пользовательский ввод из консоли
            std::cout << "Введите запрос: ";
            std::string input;
            std::getline(std::cin, input);

            // Формируем структуру запроса
            request.head.type = 1;
            request.head.len = input.size();

            request.body.buff.reserve(input.size());
            request.body.buff.resize(input.size());
            std::memcpy(request.body.buff.data(), input.c_str(), input.size());

            //Буффер для отправки
            std::vector<uint8_t> sendBuff(1024);
            asio::mutable_buffer buffer(sendBuff.data(), sendBuff.capacity());

            Request* ptrHead = reinterpret_cast<Request*>(buffer.data());
            size_t lenHead, lenBody;
            lenHead = request.head.serialize(buffer);
            lenBody = request.body.serialize(buffer);
            ptrHead->head.len = lenBody;
            sendBuff.resize(lenHead + lenBody);

            //std::vector<asio::const_buffer> buffers;


            //buffers.push_back(asio::buffer(&request.lenRequest, sizeof(request.lenRequest)));
            //buffers.push_back(asio::buffer(request.path));
            {

                try {
                // Инициализируем библиотеку asio
                asio::io_context io_context;

                // Создаем объекты для соединения с сервером и отправки данных
                tcp::socket socket(io_context);
                tcp::resolver resolver(io_context);
                asio::connect(socket, resolver.resolve("localhost", "12346"));

                // Отправляем запрос на сервер
                //asio::write(socket, buffers);
                asio::write(socket, asio::buffer(sendBuff));

                // Получаем ответ от сервера
                char response[max_length];
				ResponseHead Body;
                size_t response_length = asio::read(socket, asio::buffer(&Body, sizeof(Body)));
                std::cerr << "Ожидаем прием тела длиной: " << Body.len << std::endl;

                response_length = asio::read(socket, asio::buffer(response, Body.len));

                // Выводим ответ на экран
                std::cout << "Ответ от сервера: " << std::endl;
                
				ResponseBody body;

				asio::mutable_buffer buffer(response, Body.len);
				body.deserialize(buffer);

				for (auto& elem : body.files) {
					std::cout << "{" << elem.path << ", " << elem.type << ", " << elem.size << "}" << std::endl;
				}

                std::cout << std::endl;
                }
                catch (std::exception& e) {
                    std::cerr << "Ошибка: " << e.what() << std::endl;
                }
            }
        }


    return 0;
}