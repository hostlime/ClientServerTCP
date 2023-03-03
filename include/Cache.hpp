#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>


namespace cache{

    using keyType = std::vector<uint8_t>;
    using valueType = std::vector<uint8_t>;

    // структура данных которые будут храниться в мапе
    struct CacheItem{
        std::chrono::time_point<std::chrono::system_clock>   timeWrite; // время записи
        valueType   value;  // данные
    };

class Cache{
public:
    // Обязательный параметр при создании кеша - время жизни кеша
    Cache(std::chrono::seconds _lifeTime_) :lifeTime(_lifeTime_){}
    void insert(){
        std::lock_guard<std::mutex> lock(guard_mutex);
    }
    bool get(){
        std::lock_guard<std::mutex> lock(guard_mutex);
    }

private:
    std::unordered_map<keyType, CacheItem> cacheBase;
    std::chrono::seconds    lifeTime;           // время жизни данных в кеше
    std::mutex guard_mutex;                     // мютекс для блока записи и чтения в мапу
}

}

#endif