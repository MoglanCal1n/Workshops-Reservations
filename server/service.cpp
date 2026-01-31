#include "service.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>

WorkshopService::WorkshopService(Database& _db) : db(_db) {}

std::string WorkshopService::proceseazaRezervare(RezervareRequest req) {

    try {
        int ora_start = std::stoi(req.ora.substr(11, 2));
        
        if (ora_start < 9 || ora_start >= 17) {
            std::cout << "[DEBUG] -> Rezervare RESPINSA (Program inchis: " << ora_start << ":00)" << std::endl;
            return "{ \"status\": \"fail\", \"message\": \"Centrul este inchis. Program rezervari: 09:00 - 17:00.\" }";
        }
    } catch (...) {
        return "{ \"status\": \"fail\", \"message\": \"Format data invalid.\" }";
    }
    std::lock_guard<std::mutex> lock(db.getMutex());
    
    sqlite3* conn;
    sqlite3_open(db.dbName.c_str(), &conn);

    int max_cap = 10;
    sqlite3_stmt* stmtCap;
    std::string capSql = "SELECT capacity FROM center_capacities WHERE center_id=? AND workshop_id=?";
    sqlite3_prepare_v2(conn, capSql.c_str(), -1, &stmtCap, 0);
    sqlite3_bind_int(stmtCap, 1, req.centru_id);
    sqlite3_bind_int(stmtCap, 2, req.atelier_id);
    
    if(sqlite3_step(stmtCap) == SQLITE_ROW) {
        max_cap = sqlite3_column_int(stmtCap, 0);
    } else {
        std::cout << "[DEBUG] Nu s-a gasit capacitate in DB. Se foloseste default: " << max_cap << std::endl;
    }
    sqlite3_finalize(stmtCap);

    sqlite3_stmt* stmt;

    std::string checkSql = "SELECT count(*) FROM reservations "
                           "WHERE center_id=? AND workshop_id=? "
                           "AND strftime('%Y-%m-%d %H', reservation_time) = strftime('%Y-%m-%d %H', ?) "
                           "AND status IN ('REZERVARE', 'PLATITA')";

    sqlite3_prepare_v2(conn, checkSql.c_str(), -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, req.centru_id);
    sqlite3_bind_int(stmt, 2, req.atelier_id);
    sqlite3_bind_text(stmt, 3, req.ora.c_str(), -1, SQLITE_STATIC);
    
    int load = 0;
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        load = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    std::cout << "[DEBUG] Verificare Interval Orar: " 
              << "Centru=" << req.centru_id 
              << ", Atelier=" << req.atelier_id 
              << " pentru ora: " << req.ora.substr(0, 13)
              << " | Ocupat: " << load << "/" << max_cap << std::endl;

    std::string res;
    if(load < max_cap) {
        std::string ins = "INSERT INTO reservations (client_name, cnp, center_id, workshop_id, reservation_time) VALUES (?, ?, ?, ?, ?)";
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
        
        std::cout << "[DEBUG] -> Rezervare REUSITA (ID: " << id << ")" << std::endl;
    } else {
        res = "{ \"status\": \"fail\", \"message\": \"Locuri epuizate pentru acest interval orar (Max: " + std::to_string(max_cap) + ")\" }";
        std::cout << "[DEBUG] -> Rezervare RESPINSA (Full)" << std::endl;
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
    if(changed > 0) {
        std::string transSql = "INSERT INTO transactions (reservation_id, type, amount) VALUES (?, 'INCASARE', ?)";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(conn, transSql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_double(stmt, 2, suma);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        sqlite3_close(conn);
        return "{ \"status\": \"paid\" }";
    }
    sqlite3_close(conn);
    return "{ \"status\": \"fail\", \"message\": \"Rezervare inexistenta sau expirata\" }";
}

std::string WorkshopService::proceseazaAnulare(int id) {
    std::lock_guard<std::mutex> lock(db.getMutex());
    sqlite3* conn;
    sqlite3_open(db.dbName.c_str(), &conn);
    std::string checkSql = "SELECT amount FROM transactions WHERE reservation_id=? AND type='INCASARE'";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(conn, checkSql.c_str(), -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, id);
    double amountPaid = 0.0;
    bool found = false;
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        amountPaid = sqlite3_column_double(stmt, 0);
        found = true;
    }
    sqlite3_finalize(stmt);
    if(!found) {
        sqlite3_close(conn);
        return "{ \"status\": \"fail\", \"message\": \"Rezervarea nu este platita sau nu exista\" }";
    }
    std::string updSql = "UPDATE reservations SET status='ANULATA' WHERE id=" + std::to_string(id) + " AND status='PLATITA'";
    char* err = 0;
    sqlite3_exec(conn, updSql.c_str(), 0, 0, &err);
    if(sqlite3_changes(conn) > 0) {
        std::string refSql = "INSERT INTO transactions (reservation_id, type, amount) VALUES (?, 'RAMBURS', ?)";
        sqlite3_prepare_v2(conn, refSql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_double(stmt, 2, -amountPaid); 
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        sqlite3_close(conn);
        return "{ \"status\": \"refunded\", \"amount\": " + std::to_string(amountPaid) + " }";
    }
    sqlite3_close(conn);
    return "{ \"status\": \"fail\", \"message\": \"Eroare la anulare\" }";
}

void WorkshopService::runPeriodicChecks(int interval_secunde, int timeout_plata_secunde) {
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_secunde)); 
        
        std::lock_guard<std::mutex> lock(db.getMutex());
        sqlite3* conn;
        sqlite3_open(db.dbName.c_str(), &conn);

        std::string timeStr = "-" + std::to_string(timeout_plata_secunde) + " seconds";
        
        std::string expireSql = "UPDATE reservations SET status='EXPIRAT' WHERE status='REZERVARE' AND created_at < datetime('now', '" + timeStr + "')";
        
        char* err = 0;
        sqlite3_exec(conn, expireSql.c_str(), 0, 0, &err);
        int expiredCount = sqlite3_changes(conn);

        std::ofstream logFile("raport_audit.txt", std::ios::app);
        std::time_t t = std::time(nullptr);
        logFile << "\n[AUDIT " << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "]\n";
        
        if(expiredCount > 0) {
            logFile << "-> Curatare: " << expiredCount << " rezervari expirate au fost eliminate.\n";
            std::cout << "[AUDIT] S-au sters " << expiredCount << " rezervari expirate.\n";
        } else {
            logFile << "-> Curatare: 0 rezervari expirate.\n";
        }


        std::string statusSql = 
            "SELECT r.center_id, r.workshop_id, strftime('%Y-%m-%d %H', r.reservation_time) as ora_grup, count(*) as ocupat, cc.capacity "
            "FROM reservations r "
            "JOIN center_capacities cc ON r.center_id = cc.center_id AND r.workshop_id = cc.workshop_id "
            "WHERE r.status IN ('REZERVARE', 'PLATITA') "
            "GROUP BY r.center_id, r.workshop_id, ora_grup";
            
        sqlite3_stmt* stmtStatus;
        sqlite3_prepare_v2(conn, statusSql.c_str(), -1, &stmtStatus, 0);
        
        bool activeReservations = false;
        logFile << "-> Verificare Capacitati:\n";
        
        while(sqlite3_step(stmtStatus) == SQLITE_ROW) {
            activeReservations = true;
            int c_id = sqlite3_column_int(stmtStatus, 0);
            int w_id = sqlite3_column_int(stmtStatus, 1);
            std::string ora = (const char*)sqlite3_column_text(stmtStatus, 2);
            int ocupat = sqlite3_column_int(stmtStatus, 3);
            int cap = sqlite3_column_int(stmtStatus, 4);
            
            std::string ora_display = ora.substr(11, 2) + ":00";

            if (ocupat > cap) {
                logFile << "   [CRITIC] SUPRAPUNERE DETECTATA! Centru " << c_id << ", Atelier " << w_id 
                        << " (" << ora_display << "): " << ocupat << "/" << cap << " locuri!\n";
            } else if (ocupat == cap) {
                logFile << "   [INFO] Sala PLINA. Centru " << c_id << ", Atelier " << w_id 
                        << " (" << ora_display << "): " << ocupat << "/" << cap << ".\n";
            } else {
                logFile << "   [OK] Centru " << c_id << ", Atelier " << w_id 
                        << " (" << ora_display << "): " << ocupat << "/" << cap << ".\n";
            }
        }
        
        if(!activeReservations) {
            logFile << "   (Nicio rezervare activa in acest moment)\n";
        }
        sqlite3_finalize(stmtStatus);

        std::string auditSql = "SELECT r.center_id, SUM(t.amount) FROM transactions t JOIN reservations r ON t.reservation_id = r.id GROUP BY r.center_id";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(conn, auditSql.c_str(), -1, &stmt, 0);
        
        logFile << "-> Solduri Financiare:\n";
        bool moneyFound = false;
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            moneyFound = true;
            int center = sqlite3_column_int(stmt, 0);
            double total = sqlite3_column_double(stmt, 1);
            logFile << "   Centru " << center << ": " << total << " RON\n";
        }
        if(!moneyFound) logFile << "   0 RON incasati momentan.\n";
        sqlite3_finalize(stmt);
        
        logFile << "--------------------------------------------------\n";
        logFile.close();
        
        sqlite3_close(conn);
    }
}