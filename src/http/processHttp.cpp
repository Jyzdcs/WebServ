#include "../../include/http/processHttp.hpp"
#include "../../include/http/RequestParser.hpp"
#include "../../include/http/Router.hpp"
#include "../../include/http/MethodHandler.hpp"
#include "../../include/http/ResponseBuilder.hpp"
#include "../../include/http/builders/HttpBuilders.hpp"

ProcessResult processHttp(const std::string& rawRequest, const ServerConfig& server)
{
    ResponseBuilder builder;
    RequestParser   parser;
    ProcessResult   result;

    try
    {
        HttpRequest    request  = parser.parse(rawRequest);

        // if http0 close connexion, if http1.1 keep alive
        std::string connectionHeader = "";
        if (request.headers.count("connection"))
            connectionHeader = request.headers.find("connection")->second;

        if (request.version == "HTTP/1.0" || connectionHeader == "close")
            result.shouldClose = true;
        else
            result.shouldClose = false;

        Router         router;
        LocationConfig location = router.route(request, server);
        MethodHandler  handler;
        HttpResponse   response = handler.handle(request, location, server);
        result.response = builder.build(response, result.shouldClose);
        return result;
    }
    catch (const RequestParser::ParseException& e)
    {
        result.shouldClose = true;
        result.response    = builder.build(buildHttpError(e.getCode(), e.what()), true);
        return result;
    }
}
