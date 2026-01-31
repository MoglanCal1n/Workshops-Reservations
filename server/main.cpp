#include "crow.h"
#include "database.h"
#include "service.h"
#include "thread_pool.h"

struct CORSMiddleware {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

int main(int argc, char* argv[]) {
    int p_threads = 4;
    int audit_interval = 5;
    int plata_timeout = 12;

    if (argc >= 4) {
        p_threads = std::stoi(argv[1]);
        audit_interval = std::stoi(argv[2]);
        plata_timeout = std::stoi(argv[3]);
    }

    std::cout << "--- SERVER START ---" << std::endl;
    std::cout << "Threaduri: " << p_threads << std::endl;
    std::cout << "Interval Verificare: " << audit_interval << "s" << std::endl;
    std::cout << "Timeout Plata: " << plata_timeout << "s" << std::endl;

    Database db("workshops.db");
    db.init();
    

    WorkshopService service(db);
    ThreadPool pool(p_threads); 

    std::thread verifier([&service, audit_interval, plata_timeout]() {
        service.runPeriodicChecks(audit_interval, plata_timeout); 
    });
    verifier.detach();

    crow::App<CORSMiddleware> app;

    CROW_ROUTE(app, "/rezerva").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([&pool, &service](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) {
            return crow::response(200);
        }

        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400);

        RezervareRequest r;
        if (!x.has("nume") || !x.has("cnp") || !x.has("centru_id") || !x.has("atelier_id") || !x.has("ora")) {
            return crow::response(400, "Invalid JSON structure");
        }

        r.nume = x["nume"].s();
        r.cnp = x["cnp"].s();
        r.centru_id = x["centru_id"].i();
        r.atelier_id = x["atelier_id"].i();
        r.ora = x["ora"].s();

        auto future = pool.enqueue([&service, r] {
            return service.proceseazaRezervare(r);
        });

        crow::response resp;
        resp.body = future.get();
        return resp;
    });

    CROW_ROUTE(app, "/plateste").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([&pool, &service](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) {
            return crow::response(200);
        }

        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400);

        int id = x["id"].i();
        double suma = x["suma"].d();

        auto future = pool.enqueue([&service, id, suma] {
            return service.proceseazaPlata(id, suma);
        });

        crow::response resp;
        resp.body = future.get();
        return resp;
    });

    CROW_ROUTE(app, "/anuleaza").methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([&pool, &service](const crow::request& req){
        if (req.method == crow::HTTPMethod::Options) {
            return crow::response(200);
        }

        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400);

        int id = x["id"].i(); 

        auto future = pool.enqueue([&service, id] {
            return service.proceseazaAnulare(id);
        });

        crow::response resp;
        resp.body = future.get();
        return resp;
    });

    app.port(8080).multithreaded().run();
}