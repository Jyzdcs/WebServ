#include "../../include/http/processHttp.hpp"
#include "../../include/http/RequestParser.hpp"
#include "../../include/http/Router.hpp"
#include "../../include/http/MethodHandler.hpp"
#include "../../include/http/ResponseBuilder.hpp"
#include "../../include/http/builders/HttpBuilders.hpp"
#include <cctype>

ProcessResult processHttp(const std::string& rawRequest, const ServerConfig& server)
{
    RequestParser   parser;
    ResponseBuilder builder;
    ProcessResult   result;

    try
    {
        HttpRequest request = parser.parse(rawRequest);

        // normalize Connection value to lowercase (RFC 7230: header values are case-insensitive)
        std::string connectionHeader = "";
        if (request.headers.count("connection"))
        {
            connectionHeader = request.headers.find("connection")->second;
            for (std::size_t i = 0; i < connectionHeader.size(); i++)
                connectionHeader[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(connectionHeader[i])));
        }

        Router         router;
        LocationConfig location = router.route(request, server);
        MethodHandler  handler;
        result             = handler.handle(request, location, server);
        result.shouldClose = (request.version == "HTTP/1.0" || connectionHeader == "close");

        if (result.state == ProcessResult::COMPLETE)
            result.rawResponse = builder.build(result.httpResponse, result.shouldClose);
        return result;
    }
    catch (const RequestParser::ParseException& e)
    {
        result.state       = ProcessResult::COMPLETE;
        result.shouldClose = true;
        result.rawResponse = builder.build(buildHttpError(e.getCode(), e.what()), true);
        return result;
    }
}
