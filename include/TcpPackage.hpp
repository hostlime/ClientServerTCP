#ifndef TCP_PACKAGE_HPP
#define TCP_PACKAGE_HPP

#include <filesystem>
#include <asio/ts/buffer.hpp>
#include<CustomSerializ.hpp>


#define DEFAULT_BUFFSIZE_FOR_TCP_PACKAGE	1024



namespace TcpPackage
{
	////////////////////Список типов поддерживаемых пакетов////////////////////////
	enum PackageType : uint32_t {
		GetFileList_TYPE = 80, // Получение списка файлов по заданному пути
		TYPE_2, // пример
		TYPE_3 // пример
	};
	/// ///////////////////////////////////////////////////////////////////////////

	namespace File
	{
		struct FileDescriprion {
			//{<путь к файлу>, <тип файла>, <размер файла>}
			std::string path;
			std::string type;
			uint32_t size = 0;

			// Бинарная сериализация
			static void serialize(FileDescriprion const& value, CustomSerializ::Serializer& s) {
				//Serializer s(buffer);
				s(value.path);
				s(value.type);
				s(value.size);
			}
			// Бинарная десериализация
			static void deserialize(FileDescriprion& value, CustomSerializ::Deserializer& d) {
				//Deserializer d(buffer);
				d(value.path);
				d(value.type);
				d(value.size);
			}
		};
	}



	// Структура ЗАГОЛОВКА ответа
	struct ResponseHead {
		uint32_t type = 0;	// тип данных
		uint32_t len = 0;	// длина ответа

	public:
		size_t serialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Serializer s(buffer);
			s(type);
			s(len);
			buffer += s.offset();
			return s.offset();
		}

		// Бинарная десериализация заголовка ответа
		size_t deserialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Deserializer d(buffer);
			d(type);
			d(len);
			buffer += d.offset();
			return d.offset();
		}
	};
	// Структура ТЕЛА ответа
	struct ResponseBody {
		std::vector<File::FileDescriprion> files;

		// Бинарная сериализация  тела ответа
		std::size_t serialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Serializer s(buffer);
			uint32_t len = static_cast<uint32_t>(files.size());
			s(len);
			for (auto const& elem : files) {
				elem.serialize(elem, s);
			}
			buffer += s.offset();
			return s.offset();
		}
		void deserialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Deserializer d(buffer);
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
			CustomSerializ::Serializer s(buffer);
			s(buff);
			buffer += s.offset();
			return s.offset();
		}

		// Бинарная сериализация ЗАПРОСА
		std::size_t deserialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Deserializer d(buffer);
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
	namespace FileMethods
	{
		// Методы для работы с файлами
		class FileManager {
		public:
			// Запись списка файлов в вектор по указанному пути
			void WriteFileList(std::vector<TcpPackage::File::FileDescriprion>& files, const fs::path& directory_path) {
				files.clear();
				if (fs::is_directory(directory_path))
					for (const auto& entry : fs::directory_iterator(directory_path)) {
						// Иногда файл занят и вылетает ошибка при получении размера поэтому проверяем на занятость
						uint32_t fileSize = is_file_in_use(entry.path()) ? 0 : static_cast<uint32_t>(fs::file_size(entry.path()));
						//{<путь к файлу>, <тип файла>, <размер файла>}
						files.push_back({
							entry.path().string(),
							get_file_type(entry),
							fileSize,
							});
#ifdef _DEBUG
						// Выводим список для отладки
						const fs::path& file_path = entry.path();
						const std::string& file_type = get_file_type(entry);
						std::cout << "{" << file_path << ", " << file_type << ", " << fileSize << "}" << std::endl;
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
			// метод проверяет свободен файл или кем-то други занят
			bool is_file_in_use(const std::filesystem::path& filepath)
			{
				std::error_code ec;
				auto status = std::filesystem::status(filepath, ec);
				if (ec)
				{
					// Обработка ошибки
					return true;
				}
				return ((status.permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none);
			}
		};
	}


// Класс ответа
class Msg : FileMethods::FileManager {
public:
	void* requestHeadData() {
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
		case GetFileList_TYPE:
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
				DATA->resize(
					response.body.files.size() * sizeof(response.body.files) // размер структуры
					+ response.body.files.size() *(3*4)	// учитываем байт который добавляется при сериализации 
					+ 1024 // просто запас
				); // на всякий слачай резирвируем больше данных чтобы десереализованные данные не вышли за границу

			}
			// Сериализуем данные для дальнейшей отправки
			asio::mutable_buffer buffer(DATA->data(), DATA->size());
			Response* ptrResponse = reinterpret_cast<Response*>(buffer.data());

			lenHead = response.head.serialize(buffer);
			lenBody = response.body.serialize(buffer);

			ptrResponse->head.type = request.head.type; //копируем тип
			ptrResponse->head.len = static_cast<uint32_t>(lenBody); // записываем длину пакета
			DATA->resize(lenHead + lenBody);
			break;
			/*
		case TYPE_2:
			break;
		case TYPE_3:

			break;*/
		}

	}
	Msg() {
		request.body.buff.reserve(DEFAULT_BUFFSIZE_FOR_TCP_PACKAGE);		// Резервируем для сырых данных принятых по сети
	}
private:
	Request request;		// структура запроса
	Response response;		// структура ответа
};
}


















#endif