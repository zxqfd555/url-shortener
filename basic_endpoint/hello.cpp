#include "crow.h"

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/hello")
    ([]{
        crow::json::wvalue responseJson;
        responseJson["message"] = "Hello, World!";

        crow::response response(responseJson);
	/*
        This is how we can set response headers. In this example, we'd had to set Content-Type header of the response.
        But there's no need in that, because Crow sets this header by default, when we use JSON-based constructor.

        response.add_header("Content-Type", "application/json");
	*/

        return response;
    });

    app.loglevel(crow::LogLevel::CRITICAL);
    app.port(18080)
        .multithreaded()
        .run();
}
