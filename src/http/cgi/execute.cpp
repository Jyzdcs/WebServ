#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define CGI_TIMEOUT_SEC 5

static std::string buildScriptPath(const HttpRequest& request, const LocationConfig& location)
{
    std::string uriWithoutQuery = request.uri;
    std::size_t queryStart      = uriWithoutQuery.find('?');
    if (queryStart != std::string::npos)
        uriWithoutQuery = uriWithoutQuery.substr(0, queryStart);

    char currentWorkingDir[4096];
    if (getcwd(currentWorkingDir, sizeof(currentWorkingDir)) == NULL)
        return "";
    return std::string(currentWorkingDir) + "/" + location.getRoot() + uriWithoutQuery;
}

static void runChildProcess(const std::string& interpreter, const std::string& scriptPath,
                             char** argv, char** envp,
                             int stdinPipe[2], int stdoutPipe[2])
{
    dup2(stdinPipe[0],  STDIN_FILENO);
    dup2(stdoutPipe[1], STDOUT_FILENO);

    close(stdinPipe[0]);  close(stdinPipe[1]);
    close(stdoutPipe[0]); close(stdoutPipe[1]);

    std::string scriptDirectory = scriptPath.substr(0, scriptPath.rfind('/'));
    chdir(scriptDirectory.c_str());

    execve(interpreter.c_str(), argv, envp);
    _exit(1);
}

static std::string readCgiOutputWithTimeout(int pipeReadEnd, pid_t childPid, bool& hasTimedOut)
{
    std::string cgiOutput;
    char        readBuffer[4096];
    hasTimedOut = false;

    fcntl(pipeReadEnd, F_SETFL, O_NONBLOCK);

    time_t timeoutDeadline = time(NULL) + CGI_TIMEOUT_SEC;

    while (true)
    {
        ssize_t bytesRead = read(pipeReadEnd, readBuffer, sizeof(readBuffer));

        if (bytesRead > 0)
        {
            cgiOutput.append(readBuffer, bytesRead);
        }
        else if (bytesRead == 0)
        {
            break;
        }
        else
        {
            if (time(NULL) >= timeoutDeadline)
            {
                hasTimedOut = true;
                kill(childPid, SIGKILL);
                waitpid(childPid, NULL, 0);
                close(pipeReadEnd);
                return "";
            }
            usleep(5000);
        }
    }

    close(pipeReadEnd);
    return cgiOutput;
}

HttpResponse CgiHandler::execute(const HttpRequest& request, const LocationConfig& location)
{
    std::string scriptPath = buildScriptPath(request, location);

    if (access(scriptPath.c_str(), F_OK) == -1)
        return buildHttpError(404, "Not Found");
    if (access(scriptPath.c_str(), X_OK) == -1)
        return buildHttpError(403, "Forbidden");

    std::vector<std::string> envVars = buildEnv(request, scriptPath);
    std::vector<char*>       envPointers;
    for (std::size_t envIndex = 0; envIndex < envVars.size(); envIndex++)
        envPointers.push_back(const_cast<char*>(envVars[envIndex].c_str()));
    envPointers.push_back(NULL);

    std::string interpreter = location.getCgiPath();
    char*       argv[3]     = {
        const_cast<char*>(interpreter.c_str()),
        const_cast<char*>(scriptPath.c_str()),
        NULL
    };

    int stdinPipe[2];
    int stdoutPipe[2];
    if (pipe(stdinPipe) == -1)
        return buildHttpError(500, "Internal Server Error");
    if (pipe(stdoutPipe) == -1)
    {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return buildHttpError(500, "Internal Server Error");
    }

    pid_t childPid = fork();
    if (childPid == -1)
    {
        close(stdinPipe[0]);  close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        return buildHttpError(500, "Internal Server Error");
    }

    if (childPid == 0)
        runChildProcess(interpreter, scriptPath, argv, &envPointers[0], stdinPipe, stdoutPipe);

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    signal(SIGPIPE, SIG_IGN);

    if (!request.body.empty())
    {
        const char* bodyData          = request.body.c_str();
        std::size_t totalBytesToWrite = request.body.size();
        std::size_t totalBytesWritten = 0;

        while (totalBytesWritten < totalBytesToWrite)
        {
            ssize_t bytesWritten = write(stdinPipe[1],
                                         bodyData + totalBytesWritten,
                                         totalBytesToWrite - totalBytesWritten);
            if (bytesWritten <= 0)
                break;
            totalBytesWritten += static_cast<std::size_t>(bytesWritten);
        }
    }
    close(stdinPipe[1]);

    bool        hasTimedOut = false;
    std::string cgiOutput   = readCgiOutputWithTimeout(stdoutPipe[0], childPid, hasTimedOut);

    if (hasTimedOut)
        return buildHttpError(504, "Gateway Timeout");

    int childExitStatus = 0;
    waitpid(childPid, &childExitStatus, 0);

    if (WIFEXITED(childExitStatus) && WEXITSTATUS(childExitStatus) != 0)
        return buildHttpError(500, "Internal Server Error");

    if (WIFSIGNALED(childExitStatus))
        return buildHttpError(500, "Internal Server Error");

    if (cgiOutput.empty())
        return buildHttpError(500, "Internal Server Error");

    return parseOutput(cgiOutput);
}
