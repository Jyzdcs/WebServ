#include "../../../include/http/CgiHandler.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>

#define CGI_TIMEOUT_SEC 5

static std::string scriptPathFrom(const HttpRequest& req, const LocationConfig& loc)
{
    std::string uri = req.uri;
    std::size_t q = uri.find('?');
    if (q != std::string::npos)
        uri = uri.substr(0, q);

    char cwd[4096];
    getcwd(cwd, sizeof(cwd));
    return std::string(cwd) + "/" + loc.getRoot() + uri;
}

static void runChild(const std::string& interpreter, const std::string& scriptPath,
                     char** argv, char** envp,
                     int stdin_pipe[2], int stdout_pipe[2])
{
    dup2(stdin_pipe[0],  STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);

    close(stdin_pipe[0]);  close(stdin_pipe[1]);
    close(stdout_pipe[0]); close(stdout_pipe[1]);

    std::string dir = scriptPath.substr(0, scriptPath.rfind('/'));
    chdir(dir.c_str());

    execve(interpreter.c_str(), argv, envp);
    _exit(1);
}

static std::string readOutputWithTimeout(int fd, pid_t pid)
{
    std::string output;
    char        buf[4096];

    while (true)
    {
        fd_set         readfds;
        struct timeval timeout;

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec  = CGI_TIMEOUT_SEC;
        timeout.tv_usec = 0;

        int ready = select(fd + 1, &readfds, NULL, NULL, &timeout);

        if (ready == 0)
        {
            // timeout : tuer le script
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            close(fd);
            return "";
        }
        if (ready == -1)
            break;

        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0)
            break;
        output.append(buf, n);
    }
    close(fd);
    return output;
}

HttpResponse CgiHandler::execute(const HttpRequest& req, const LocationConfig& loc)
{
    std::string scriptPath = scriptPathFrom(req, loc);

    if (access(scriptPath.c_str(), F_OK) == -1)
        return buildError(404, "Not Found");
    if (access(scriptPath.c_str(), X_OK) == -1)
        return buildError(403, "Forbidden");

    std::vector<std::string> envVars = buildEnv(req, scriptPath);
    std::vector<char*>       envp;
    for (std::size_t i = 0; i < envVars.size(); i++)
        envp.push_back(const_cast<char*>(envVars[i].c_str()));
    envp.push_back(NULL);

    std::string interpreter = loc.getCgiPath();
    char* argv[3] = {
        const_cast<char*>(interpreter.c_str()),
        const_cast<char*>(scriptPath.c_str()),
        NULL
    };

    int stdin_pipe[2];
    int stdout_pipe[2];
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1)
        return buildError(500, "Internal Server Error");

    pid_t pid = fork();
    if (pid == -1)
    {
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        return buildError(500, "Internal Server Error");
    }

    if (pid == 0)
        runChild(interpreter, scriptPath, argv, envp.data(), stdin_pipe, stdout_pipe);

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);

    if (!req.body.empty())
        write(stdin_pipe[1], req.body.c_str(), req.body.size());
    close(stdin_pipe[1]);

    std::string output = readOutputWithTimeout(stdout_pipe[0], pid);

    if (output.empty())
    {
        // timeout ou sortie vide — le waitpid a déjà été fait dans readOutputWithTimeout
        return buildError(504, "Gateway Timeout");
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        return buildError(500, "Internal Server Error");

    return parseOutput(output);
}
