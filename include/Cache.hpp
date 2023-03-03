#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace cache
{

    class Cache
    {

        using keyType = std::string;
        using valueType = std::vector<uint8_t>;

        // структура данных которые будут храниться в мапе
        struct CacheItem
        {
            valueType data;                                               // данные
            std::chrono::time_point<std::chrono::system_clock> timeWrite; // время записи
        };

    public:
        // Обязательный параметр при создании кеша - время жизни кеша
        Cache(std::chrono::seconds _lifeTime_) : lifeTime(_lifeTime_) {}
        void insert(keyType &key, valueType &value)
        {
            std::lock_guard<std::mutex> lock(guard_mutex);
            cacheBase[key] = {value,
                              std::chrono::system_clock::now()};
        }
        bool get(keyType &key, valueType &outData)
        {
            std::lock_guard<std::mutex> lock(guard_mutex);

            auto it = cacheBase.find(key);
            if (it != cacheBase.end())
            {
                // Данные есть в кеше
                // теперь проверяем время
                if ((std::chrono::system_clock::now() - it->second.timeWrite) < lifeTime)
                {
                    outData = it->second.data;
                    return true;
                }
                cacheBase.erase(it); // Время истекло поэтому удяляем этот элемент
            }
            return false;
        }

    private:
        std::unordered_map<keyType, CacheItem> cacheBase;
        std::chrono::seconds lifeTime; // время жизни данных в кеше
        std::mutex guard_mutex;        // мютекс для блока записи и чтения в мапу
    };
}

#endif