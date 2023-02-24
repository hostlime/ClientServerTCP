#include <iostream>
#include <vector>
#include <asio.hpp>

class Serializer {
public:
    Serializer(asio::mutable_buffer buffer) : buffer_(buffer), offset_(0) {}

    template<typename T>
    void operator()(T const& value) {
        std::memcpy(buffer_.data() + offset_, &value, sizeof(value));
        offset_ += sizeof(value);
    }

    void operator()(std::string const& value) {
        uint32_t len = static_cast<uint32_t>(value.size());
        (*this)(len);
        std::memcpy(buffer_.data() + offset_, value.data(), len);
        offset_ += len;
    }

    template<typename T>
    void operator()(std::vector<T> const& value) {
        uint32_t len = static_cast<uint32_t>(value.size());
        (*this)(len);
        for (auto const& elem : value) {
            (*this)(elem);
        }
    }

    void reset() {
        offset_ = 0;
    }

private:
    asio::mutable_buffer buffer_;
    std::size_t offset_;
};

class Deserializer {
public:
    Deserializer(asio::const_buffer buffer) : buffer_(buffer), offset_(0) {}

    template<typename T>
    void operator()(T& value) {
        std::memcpy(&value, buffer_.data() + offset_, sizeof(value));
        offset_ += sizeof(value);
    }

    void operator()(std::string& value) {
        uint32_t len;
        (*this)(len);
        value.resize(len);
        std::memcpy(value.data(), buffer_.data() + offset_, len);
        offset_ += len;
    }

    template<typename T>
    void operator()(std::vector<T>& value) {
        uint32_t len;
        (*this)(len);
        value.resize(len);
        for (auto& elem : value) {
            (*this)(elem);
        }
    }

    void reset() {
        offset_ = 0;
    }

private:
    asio::const_buffer buffer_;
    std::size_t offset_;
};

struct FileDescriprion {
    std::string path;
    std::string type;
    uint32_t size;

    // Бинарная сериализация
    friend asio::mutable_buffer serialize(FileDescriprion const& value, asio::mutable_buffer buffer) {
        Serializer s(buffer);
        s(value.path);
        s(value.type);
        s(value.size);
        return buffer + s.offset();
    }

    // Бинарная десериализация
    friend asio::const_buffer deserialize(FileDescriprion& value, asio::const_buffer buffer) {
        Deserializer d(buffer);
        d(value.path);
        d(value.type);
        d(value.size);
        return buffer + d.offset();
    }
};

// Бинарная сериализация заголовка ответа
friend asio::mutable_buffer serialize(ResponseHead const& value, asio::mutable_buffer buffer) {
    Serializer s(buffer);
    s(value.type);
    s(value.len);
    return buffer + s.offset();
}

// Бинарная десериализация заголовка ответа
friend asio::const_buffer deserialize(ResponseHead& value, asio::const_buffer buffer) {
    Deserializer d(buffer);
    d(value.type);
    d(value.len);
    return buffer + d.offset();
}
