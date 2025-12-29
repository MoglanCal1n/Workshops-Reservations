#include "database.h"
#include <iostream>

Database::Database(const std::string& name) : dbName(name) {}

Database::~Database() {}

std::mutex& Database::getMutex() {
    return db_mutex;
}

void Database::execute(const std::string& sql) {
    sqlite3* db;
    if (sqlite3_open(dbName.c_str(), &db)) return;

    char* err = 0;
    sqlite3_exec(db, sql.c_str(), 0, 0, &err);

    if (err) {
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
    }
    sqlite3_close(db);
}

void Database::init() {
    execute("PRAGMA journal_mode=WAL;");

    const std::string schema = R"(
        CREATE TABLE IF NOT EXISTS center_capacities (
            center_id INTEGER, workshop_id INTEGER, capacity INTEGER NOT NULL,
            PRIMARY KEY (center_id, workshop_id)
        );
        CREATE TABLE IF NOT EXISTS reservations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            client_name TEXT, cnp TEXT, center_id INTEGER, workshop_id INTEGER,
            reservation_time TEXT, status TEXT DEFAULT 'REZERVARE',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT, reservation_id INTEGER,
            type TEXT, amount REAL, timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    execute(schema);
    
    execute("INSERT OR IGNORE INTO center_capacities VALUES (1, 1, 10), (1, 2, 15);");
}