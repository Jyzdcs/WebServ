#include "../../include/http/processHttp.hpp"
#include "../../include/http/RequestParser.hpp"
#include "../../include/http/Router.hpp"
#include "../../include/http/MethodHandler.hpp"
#include "../../include/http/ResponseBuilder.hpp"
#include "../../include/http/utils/HttpUtils.hpp"

std::string processHttp(const std::string& rawRequest, const ServerConfig& server)
{
    ResponseBuilder builder;
    RequestParser   parser;

    try
    {
        HttpRequest    request  = parser.parse(rawRequest);
        Router         router;
        LocationConfig location = router.route(request, server);
        MethodHandler  handler;
        HttpResponse   response = handler.handle(request, location, server);
        return builder.build(response);
    } 
    catch (const RequestParser::ParseException& e)
    {
        return builder.build(buildHttpError(e.getCode(), e.what()));
    }
}
