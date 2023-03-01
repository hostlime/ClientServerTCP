#ifndef TCP_PACKAGE_HPP
#define TCP_PACKAGE_HPP

#define DEFAULT_BUFFSIZE_FOR_TCP_PACKAGE	1024



namespace TcpPackage
{
	namespace File
	{
		struct FileDescriprion {
			//{<���� � �����>, <��� �����>, <������ �����>}
			std::string path;
			std::string type;
			UINT32 size = 0;

			// �������� ������������
			static void serialize(FileDescriprion const& value, CustomSerializ::Serializer& s) {
				//Serializer s(buffer);
				s(value.path);
				s(value.type);
				s(value.size);
			}
			// �������� ��������������
			static void deserialize(FileDescriprion& value, CustomSerializ::Deserializer& d) {
				//Deserializer d(buffer);
				d(value.path);
				d(value.type);
				d(value.size);
			}
		};
	}



	// ��������� ��������� ������
	struct ResponseHead {
		uint32_t type = 0;	// ��� ������
		uint32_t len = 0;	// ����� ������

	public:
		size_t serialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Serializer s(buffer);
			s(type);
			s(len);
			buffer += s.offset();
			return s.offset();
		}

		// �������� �������������� ��������� ������
		size_t deserialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Deserializer d(buffer);
			d(type);
			d(len);
			buffer += d.offset();
			return d.offset();
		}
	};
	// ��������� ���� ������
	struct ResponseBody {
		std::vector<File::FileDescriprion> files;

		// �������� ������������  ���� ������
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
			d(len); // ��������� ���������� ������
			files.resize(len);
			for (auto& elem : files) {
				elem.deserialize(elem, d);
			}
			buffer += d.offset();
			// return d.offset(); �������������� ��������� �������� ����� �� ������
		}
	};
	struct RequestBody {
		std::vector<uint8_t> buff;

		// �������� ������������ �������
		std::size_t serialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Serializer s(buffer);
			s(buff);
			buffer += s.offset();
			return s.offset();
		}

		// �������� ������������ �������
		std::size_t deserialize(asio::mutable_buffer& buffer) {
			CustomSerializ::Deserializer d(buffer);
			d(buff);

			buffer += d.offset();
			return buff.size();	// ������ �� d.offset() ? 
			// ���� � ��� ��� ��� �������������� ���������� ������ �� ������ �� ����� offset()
			// offset() ��������� � ���������� ���� ���������� �� ����� ����� ������
		}
	};

	// ��������� �������
	struct Request {
		ResponseHead head;
		RequestBody body;
	};
	// ��������� ������
	struct Response {
		ResponseHead head;
		ResponseBody body;
	};


namespace fs = std::filesystem;
	namespace FileMethods
	{
		// ������ ��� ������ � �������
		class FileManager {
		public:
			// ������ ������ ������ � ������ �� ���������� ����
			void WriteFileList(std::vector<TcpPackage::File::FileDescriprion>& files, const fs::path& directory_path) {
				files.clear();
				if (fs::is_directory(directory_path))
					for (const auto& entry : fs::directory_iterator(directory_path)) {
						//{<���� � �����>, <��� �����>, <������ �����>}
						files.push_back({
							entry.path().string(),
							get_file_type(entry),
							static_cast<uint32_t>(fs::file_size(entry.path())),
							});
#ifdef _DEBUG
						// ������� ������ ��� �������
						const fs::path& file_path = entry.path();
						const std::string& file_type = get_file_type(entry);
						const uintmax_t file_size = fs::file_size(file_path);
						std::cout << "{" << file_path << ", " << file_type << ", " << file_size << "}" << std::endl;
#endif
					}

			}
		private:
			// ����� ��� ��������� ���� ������ �����
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
	}


// ����� ������
class Msg : FileMethods::FileManager {
public:
	void* requestHeadData() {
		return &request.head;
	}
	size_t requestHeadSizeof() {
		return sizeof(request.head);
	}

	std::vector<uint8_t>* requestBodyData() {
		// ��� ����� ���� �� ��������������� ��������� ��� ��������� ����� ����
		// �� � ���� ��� ������ �.� ������ ��������� ������� �� ������� ����� ������
		// � ������������ ��� �� ������

		request.body.buff.resize(request.head.len);
		return &request.body.buff;
	}
	void makeResponse(std::shared_ptr<std::vector<uint8_t>> DATA)
	{
		switch (request.head.type) {
		case 1:
			size_t lenHead, lenBody;
			//************************������������� ���� �������***************************
			{
				DATA->resize(1024);
				asio::mutable_buffer buffer(request.body.buff.data(), request.body.buff.size());// ������������ ���� ������ � mutable_buffer(��� ��������)
				lenBody = request.body.deserialize(buffer);
				// �������������� array � ������
				std::string spath(request.body.buff.begin(), request.body.buff.end());
				fs::path file_path = fs::path(spath);
				// ��������� ���������� � ������ �� ���� file_path
				WriteFileList(response.body.files, file_path);
				DATA->resize(
					response.body.files.size() * sizeof(response.body.files) // ������ ���������
					+ response.body.files.size() *(3*4)	// ��������� ���� ������� ����������� ��� ������������ 
					+ 1024 // ������ �����
				); // �� ������ ������ ����������� ������ ������ ����� ����������������� ������ �� ����� �� �������

			}
			// ����������� ������ ��� ���������� ��������
			asio::mutable_buffer buffer(DATA->data(), DATA->size());
			Response* ptrResponse = reinterpret_cast<Response*>(buffer.data());

			lenHead = response.head.serialize(buffer);
			lenBody = response.body.serialize(buffer);

			ptrResponse->head.type = request.head.type; //�������� ���
			ptrResponse->head.len = static_cast<uint32_t>(lenBody); // ���������� ����� ������
			DATA->resize(lenHead + lenBody);
			break;
		}

	}
	Msg() {
		request.body.buff.reserve(DEFAULT_BUFFSIZE_FOR_TCP_PACKAGE);		// ����������� ��� ����� ������ �������� �� ����
	}
private:
	Request request;		// ��������� �������
	Response response;		// ��������� ������
};
}


















#endif