#include "crow.h"
#include "database.h"
#include "service.h"
#include "thread_pool.h"

int main() {
    Database db("workshops.db");
    db.init();

    WorkshopService service(db);
    ThreadPool pool(4);

    std::thread verifier([&service]() {
        service.runPeriodicChecks();
    });
    verifier.detach();

    crow::SimpleApp app;

    CROW_ROUTE(app, "/rezerva").methods(crow::HTTPMethod::POST)
    ([&pool, &service](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400);

        RezervareRequest r;
        r.nume = x["nume"].s();
        r.cnp = x["cnp"].s();
        r.centru_id = x["centru_id"].i();
        r.atelier_id = x["atelier_id"].i();
        r.ora = x["ora"].s();

        auto future = pool.enqueue([&service, r] {
            return service.proceseazaRezervare(r);
        });

        crow::response resp;
        resp.add_header("Access-Control-Allow-Origin", "*");
        resp.body = future.get();
        return resp;
    });

    CROW_ROUTE(app, "/plateste").methods(crow::HTTPMethod::POST)
    ([&pool, &service](const crow::request& req){
        auto x = crow::json::load(req.body);
        int id = x["id"].i();
        double suma = x["suma"].d();

        auto future = pool.enqueue([&service, id, suma] {
            return service.proceseazaPlata(id, suma);
        });

        crow::response resp;
        resp.add_header("Access-Control-Allow-Origin", "*");
        resp.body = future.get();
        return resp;
    });

    CROW_ROUTE(app, "/rezerva").methods(crow::HTTPMethod::OPTIONS)([](){ return crow::response(200); });
    CROW_ROUTE(app, "/plateste").methods(crow::HTTPMethod::OPTIONS)([](){ return crow::response(200); });

    app.port(8080).multithreaded().run();
}