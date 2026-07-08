#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <ctime>

#define CGI_TIMEOUT_SEC 5

static std::string buildScriptPath(const HttpRequest& request, const LocationConfig& location)
{
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return "";
    return std::string(cwd) + "/" + location.getRoot() + extractUriPath(request.uri);
}

static ProcessResult makeError(int code, const std::string& msg)
{
    ProcessResult err;
    err.state        = ProcessResult::COMPLETE;
    err.httpResponse = buildHttpError(code, msg);
    return err;
}

static void runChild(const std::string& interpreter, const std::string& scriptPath,
                     char** argv, char** envp,
                     int stdinPipe[2], int stdoutPipe[2])
{
    dup2(stdinPipe[0],  STDIN_FILENO);
    dup2(stdoutPipe[1], STDOUT_FILENO);

    close(stdinPipe[0]);  close(stdinPipe[1]);
    close(stdoutPipe[0]); close(stdoutPipe[1]);

    std::string scriptDir = scriptPath.substr(0, scriptPath.rfind('/'));
    chdir(scriptDir.c_str());

    execve(interpreter.c_str(), argv, envp);
    _exit(1);
}

// Non-bloquant : retourne CGI_PENDING (fd+pid+deadline) si le script est lancé,
// COMPLETE (httpResponse avec l'erreur) si 404/403/500 avant le fork.
ProcessResult CgiHandler::start(const HttpRequest& request, const LocationConfig& location)
{
    std::string scriptPath = buildScriptPath(request, location);

    if (access(scriptPath.c_str(), F_OK) == -1)
        return makeError(404, "Not Found");
    if (access(scriptPath.c_str(), X_OK) == -1)
        return makeError(403, "Forbidden");

    std::vector<std::string> envVars = buildEnv(request, scriptPath);
    std::vector<char*>       envPtrs;
    for (std::size_t i = 0; i < envVars.size(); i++)
        envPtrs.push_back(const_cast<char*>(envVars[i].c_str()));
    envPtrs.push_back(NULL);

    std::string interpreter = location.getCgiPath();
    char* argv[3] = {
        const_cast<char*>(interpreter.c_str()),
        const_cast<char*>(scriptPath.c_str()),
        NULL
    };

    int stdinPipe[2];
    int stdoutPipe[2];
    if (pipe(stdinPipe) == -1)
        return makeError(500, "Internal Server Error");
    if (pipe(stdoutPipe) == -1)
    {
        close(stdinPipe[0]); close(stdinPipe[1]);
        return makeError(500, "Internal Server Error");
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        close(stdinPipe[0]);  close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        return makeError(500, "Internal Server Error");
    }

    if (pid == 0)
        runChild(interpreter, scriptPath, argv, &envPtrs[0], stdinPipe, stdoutPipe);

    close(stdinPipe[0]);
    close(stdoutPipe[1]);
    signal(SIGPIPE, SIG_IGN);

    if (!request.body.empty())
        writeFdFromString(stdinPipe[1], request.body);
    close(stdinPipe[1]);

    fcntl(stdoutPipe[0], F_SETFL, O_NONBLOCK);

    ProcessResult result;
    result.state    = ProcessResult::CGI_PENDING;
    result.stdoutFd = stdoutPipe[0];
    result.pid      = pid;
    result.deadline = time(NULL) + CGI_TIMEOUT_SEC;
    return result;
}
