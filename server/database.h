#pragma once
#include <string>
#include <mutex>
#include "sqlite3.h"

class Database {
    public:
        Database(const std::string& db_name);
        ~Database();

        void init();
        void execute(const std::string& sql);
        std::mutex& getMutex();
        std::string dbName;

    private:
        std::mutex db_mutex;
};