#include "service.h"
#include <iostream>
#include <thread>
#include <chrono>

WorkshopService::WorkshopService(Database& _db) : db(_db) {}

std::string WorkshopService::proceseazaRezervare(RezervareRequest req) {
    std::lock_guard<std::mutex> lock(db.getMutex());
    
    sqlite3* conn;
    sqlite3_open(db.dbName.c_str(), &conn);

    sqlite3_stmt* stmt;
    std::string checkSql = "SELECT count(*) FROM reservations WHERE center_id=? AND workshop_id=? AND reservation_time=? AND status IN ('REZERVARE', 'PLATITA')";
    sqlite3_prepare_v2(conn, checkSql.c_str(), -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, req.centru_id);
    sqlite3_bind_int(stmt, 2, req.atelier_id);
    sqlite3_bind_text(stmt, 3, req.ora.c_str(), -1, SQLITE_STATIC);
    
    int load = 0;
    if(sqlite3_step(stmt) == SQLITE_ROW) load = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    int max_cap = 10; 

    std::string res;
    if(load < max_cap) {
        std::string ins = "INSERT INTO reservations(client_name, cnp, center_id, workshop_id, reservation_time) VALUES (?,?,?,?,?)";
        sqlite3_prepare_v2(conn, ins.c_str(), -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, req.nume.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, req.cnp.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, req.centru_id);
        sqlite3_bind_int(stmt, 4, req.atelier_id);
        sqlite3_bind_text(stmt, 5, req.ora.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        
        long long id = sqlite3_last_insert_rowid(conn);
        res = "{ \"status\": \"success\", \"id\": " + std::to_string(id) + " }";
        sqlite3_finalize(stmt);
    } else {
        res = "{ \"status\": \"fail\", \"message\": \"Full\" }";
    }

    sqlite3_close(conn);
    return res;
}

std::string WorkshopService::proceseazaPlata(int id, double suma) {
    std::lock_guard<std::mutex> lock(db.getMutex());
    
    sqlite3* conn;
    sqlite3_open(db.dbName.c_str(), &conn);
    
    std::string sql = "UPDATE reservations SET status='PLATITA' WHERE id=" + std::to_string(id) + " AND status='REZERVARE'";
    char* err = 0;
    sqlite3_exec(conn, sql.c_str(), 0, 0, &err);
    
    int changed = sqlite3_changes(conn);
    sqlite3_close(conn);

    if(changed > 0) return "{ \"status\": \"paid\" }";
    return "{ \"status\": \"error\" }";
}

void WorkshopService::runPeriodicChecks() {
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::mutex> lock(db.getMutex());
        
        db.execute("UPDATE reservations SET status='EXPIRATA' WHERE status='REZERVARE' AND (strftime('%s','now') - strftime('%s', created_at)) > 12");
        std::cout << "[AUDIT] Verificare periodica efectuata." << std::endl;
    }
}