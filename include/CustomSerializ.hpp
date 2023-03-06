#ifndef CUSTOM_SERILIALIZATION_HPP
#define CUSTOM_SERILIALIZATION_HPP

#include <vector>
#include <asio.hpp>

namespace CustomSerializ
{
	class Serializer
	{
	public:
		Serializer(asio::mutable_buffer buffer) : buffer_(buffer), offset_(0) {}

		template <typename T>
		void operator()(T const &value)
		{
			// В задумке memcpy не должен был работать с nullptr,
			// но т.к мы к buffer_.data() добавляем offset_ то приходится проверять самостоятельно
			// на первый взгляд это кажется не самым лучшим решением, но мы теряем всего лишь несколько тактов процессорного времени
			// т.к зачастую проверку на nullptr можно осуществить одной командой процессора
			if (buffer_.data() != nullptr)
				std::memcpy((static_cast<uint8_t *>(buffer_.data()) + offset_), &value, sizeof(T));
			offset_ += sizeof(T);
		}
		void operator()(std::string const &value)
		{
			uint32_t len = static_cast<uint32_t>(value.size());
			(*this)(len);
			if (buffer_.data() != nullptr)
				std::memcpy((void *)((size_t)buffer_.data() + offset_), value.data(), len);
			offset_ += len;
		}

		template <typename T>
		void operator()(std::vector<T> const &value)
		{
			uint32_t len = static_cast<uint32_t>(value.size());
			(*this)(len);
			for (auto const &elem : value)
			{
				(*this)(elem);
			}
		}

		void reset()
		{
			offset_ = 0;
		}
		std::size_t offset()
		{
			return offset_;
		}

	private:
		asio::mutable_buffer buffer_;
		std::size_t offset_;
	};

	class Deserializer
	{
	public:
		Deserializer(asio::const_buffer buffer) : buffer_(buffer), offset_(0) {}

		template <typename T>
		void operator()(T &value)
		{
			std::memcpy(&value, (const void *)((size_t)buffer_.data() + offset_), sizeof(value));
			offset_ += sizeof(value);
		}

		void operator()(std::string &value)
		{
			uint32_t len;
			(*this)(len);
			value.resize(len);
			std::memcpy(value.data(), (const void *)((size_t)buffer_.data() + offset_), len);
			offset_ += len;
		}

		template <typename T>
		void operator()(std::vector<T> &value)
		{
			uint32_t len;
			(*this)(len);
			value.resize(len);
			for (auto &elem : value)
			{
				(*this)(elem);
			}
		}

		void reset()
		{
			offset_ = 0;
		}

		std::size_t offset()
		{
			return offset_;
		}

	private:
		asio::const_buffer buffer_;
		std::size_t offset_;
	};
}
#endif