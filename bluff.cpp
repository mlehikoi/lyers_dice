#include "crow.h"
//#include "crow_all.h"

#include "json.hpp"
#include "helpers.hpp"
#include "engine.hpp"

#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.


using namespace std;
using namespace dice;

namespace dice {
inline auto readFile(const std::string& name)
{
    const auto path = "../static/"s + name;
    const auto data = dice::slurp("../static/"s + path);
    if (data.empty()) return crow::response(404);
    crow::response r{data};
    r.add_header("Content-Type", getContentType(name));
    return r;
}
} // dice

int main()
{
    static dice::Engine engine{"db.json"};
    
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]{ return readFile("index.html"); });
    
    //@TODO This isn't login but register
    CROW_ROUTE(app, "/api/login")
        .methods("POST"_method)
        ([](const crow::request& req)
        {
            return engine.login(req.body);
        }
    );
    
    CROW_ROUTE(app, "/api/status")
        .methods("POST"_method)
        ([](const crow::request& req)
        {
            return engine.status(req.body);
        }
    );
    
    CROW_ROUTE(app, "/api/newGame")
        .methods("POST"_method)
        ([](const crow::request& req)
        {
            return engine.createGame(req.body);
        }
    );

    CROW_ROUTE(app, "/api/join")
        .methods("POST"_method)
        ([](const crow::request& req)
        {
            return engine.joinGame(req.body);
        }
    );

    CROW_ROUTE(app, "/api/startGame")
    .methods("POST"_method)
    ([](const crow::request& req)
    {
        auto r = engine.startGame(req.body);
        std::cout << "Return " << r << endl;
        return r;
    });

    CROW_ROUTE(app, "/api/startRound")
    .methods("POST"_method)
    ([](const crow::request& req)
    {
        auto r = engine.startRound(req.body);
        std::cout << "Return " << r << endl;
        return r;
    });

    CROW_ROUTE(app, "/api/bid")
    .methods("POST"_method)
    ([](const crow::request& req)
    {
        auto r = engine.bid(req.body);
        std::cout << "Return " << r << endl;
        return r;
    });

    CROW_ROUTE(app, "/api/challenge")
    .methods("POST"_method)
    ([](const crow::request& req)
    {
        auto r = engine.challenge(req.body);
        std::cout << "Return " << r << endl;
        return r;
    });

    CROW_ROUTE(app, "/api/games")([]{
        const auto data = engine.getGames();
        return data;
        // crow::response r{data};
        // r.add_header("Content-Type", "application/json; charset=utf-8");
        // return r;
    });

    CROW_ROUTE(app, "/<string>")(readFile);
    
    app.port(8000).run();
}
