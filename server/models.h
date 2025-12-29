#pragma once
#include <string>

struct RezervareRequest {
    std::string nume;
    std::string cnp;
    std::string ora;
    int centru_id;
    int atelier_id;
};