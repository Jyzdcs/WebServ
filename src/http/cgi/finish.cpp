#include "../../../include/http/processHttp.hpp"
#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/ResponseBuilder.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include <sys/wait.h>

// Appelée par le Core Server quand le pipe est EOF ou deadline dépassée.
// timedOut=true : Core Server a déjà kill(pid, SIGKILL).
// Retourne une string HTTP prête à envoyer — le serveur n'a rien à faire.
std::string finishCgi(const std::string& output, pid_t pid, bool timedOut,
                      bool shouldClose, const ServerConfig& server)
{
    (void)server;
    ResponseBuilder builder;

    if (timedOut)
    {
        waitpid(pid, NULL, 0);
        return builder.build(buildHttpError(504, "Gateway Timeout"), true);
    }

    int exitStatus = 0;
    waitpid(pid, &exitStatus, 0);

    if (WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) != 0)
        return builder.build(buildHttpError(500, "Internal Server Error"), true);
    if (WIFSIGNALED(exitStatus))
        return builder.build(buildHttpError(500, "Internal Server Error"), true);

    if (output.empty())
        return builder.build(buildHttpError(500, "Internal Server Error"), true);

    CgiHandler handler;
    return builder.build(handler.parseOutput(output), shouldClose);
}
