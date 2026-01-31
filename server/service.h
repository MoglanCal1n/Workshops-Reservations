#pragma once
#include "database.h"
#include "models.h"
#include <string>

class WorkshopService {
    public:
        WorkshopService(Database& db);

        std::string proceseazaRezervare(RezervareRequest req);
        std::string proceseazaPlata(int id, double suma);
        std::string proceseazaAnulare(int id);
        void runPeriodicChecks(int interval_secunde, int timeout_plata_secunde);
    private:
        Database& db;
};