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

int main() {
    Database db("workshops.db");
    db.init();

    WorkshopService service(db);
    ThreadPool pool(4);

    std::thread verifier([&service]() {
        service.runPeriodicChecks();
    });
    verifier.detach();

    crow::App<CORSMiddleware> app;

    CROW_ROUTE(app, "/rezerva").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)
    ([&pool, &service](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
            return crow::response(200); // Middleware-ul va atasa headerele CORS automat
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

    CROW_ROUTE(app, "/plateste").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)
    ([&pool, &service](const crow::request& req){
        if (req.method == crow::HTTPMethod::OPTIONS) {
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

    app.port(8080).multithreaded().run();
}