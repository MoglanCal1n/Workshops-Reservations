#pragma once
#include "database.h"
#include "models.h"
#include <string>

class WorkshopService {
    public:
        WorkshopService(Database& db);

        std::string proceseazaRezervare(RezervareRequest req);
        std::string proceseazaPlata(int id, double suma);
        void runPeriodicChecks();

    private:
        Database& db;
};