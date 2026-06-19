#include "../../../include/http/CgiHandler.hpp"
#include "../../../include/http/builders/HttpBuilders.hpp"
#include "../../../include/http/utils/StringUtils.hpp"
#include "../../../include/http/utils/HttpUtils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

// durée maximale d'exécution d'un script CGI avant SIGKILL + 504
#define CGI_TIMEOUT_SEC 5

// vérifie que le script existe (404) et est exécutable (403)
// retourne une réponse vide (status_code == 0) si tout est OK
static HttpResponse validateScript(const std::string& scriptPath)
{
    if (access(scriptPath.c_str(), F_OK) == -1)
        return buildHttpError(404, "Not Found");
    if (access(scriptPath.c_str(), X_OK) == -1)
        return buildHttpError(403, "Forbidden");
    HttpResponse ok;
    return ok;
}

// inspecte le code de sortie du process enfant après waitpid()
// retourne une réponse vide si le process s'est terminé normalement (exit 0)
static HttpResponse checkChildStatus(int exitStatus)
{
    // Erreur: le script a retourné un code non-zéro
    if (WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) != 0)
        return buildHttpError(500, "Internal Server Error");
    // Erreur: le script a été tué par un signal (ex: SIGSEGV)
    if (WIFSIGNALED(exitStatus))
        return buildHttpError(500, "Internal Server Error");
    HttpResponse ok;
    return ok;
}

// construit le chemin absolu du script
// ex: cwd="/srv/webserv" + root="www" + uri="/cgi-bin/hello.py" → "/srv/webserv/www/cgi-bin/hello.py"
static std::string buildScriptPath(const HttpRequest& request, const LocationConfig& location)
{
    char currentWorkingDir[4096];
    if (getcwd(currentWorkingDir, sizeof(currentWorkingDir)) == NULL)
        return "";
    return std::string(currentWorkingDir) + "/" + location.getRoot() + extractUriPath(request.uri);
}

// exécuté dans le process enfant après fork() :
// redirige stdin/stdout vers les pipes, change de répertoire, puis execve le script
static void runChildProcess(const std::string& interpreter, const std::string& scriptPath,
                             char** argv, char** envp,
                             int stdinPipe[2], int stdoutPipe[2])
{
    // redirige stdin/stdout vers les pipes
    dup2(stdinPipe[0],  STDIN_FILENO);
    dup2(stdoutPipe[1], STDOUT_FILENO);

    close(stdinPipe[0]);  close(stdinPipe[1]);
    close(stdoutPipe[0]); close(stdoutPipe[1]);

    // se place dans le répertoire du script (certains scripts utilisent des chemins relatifs)
    std::string scriptDirectory = scriptPath.substr(0, scriptPath.rfind('/'));
    chdir(scriptDirectory.c_str());

    execve(interpreter.c_str(), argv, envp);
    _exit(1);
}

// lit la sortie du script CGI avec un timeout
// utilise un read() non-bloquant + usleep pour éviter un second poll()
// si le timeout est dépassé : SIGKILL + sets hasTimedOut = true + retourne ""
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
            // données disponibles : on accumule
            cgiOutput.append(readBuffer, bytesRead);
        }
        else if (bytesRead == 0)
        {
            // EOF : le script a fermé son stdout, on a tout lu
            break;
        }
        else
        {
            // EAGAIN : pas encore de données, on vérifie le timeout
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
    // 1. valider le script
    std::string  scriptPath  = buildScriptPath(request, location);
    HttpResponse scriptError = validateScript(scriptPath);
    if (scriptError.status_code != 0)
        return scriptError;

    // 2. préparer les variables d'environnement CGI et argv
    std::vector<std::string> envVars = buildEnv(request, scriptPath);
    std::vector<char*>       envPointers;
    for (std::size_t i = 0; i < envVars.size(); i++)
        envPointers.push_back(const_cast<char*>(envVars[i].c_str()));
    envPointers.push_back(NULL);

    std::string interpreter = location.getCgiPath();
    char*       argv[3]     = {
        const_cast<char*>(interpreter.c_str()),
        const_cast<char*>(scriptPath.c_str()),
        NULL
    };

    // 3. créer les pipes stdin/stdout
    int stdinPipe[2];
    int stdoutPipe[2];
    if (pipe(stdinPipe) == -1)
        return buildHttpError(500, "Internal Server Error");
    if (pipe(stdoutPipe) == -1)
    {
        close(stdinPipe[0]); close(stdinPipe[1]);
        return buildHttpError(500, "Internal Server Error");
    }

    // 4. fork : le child exécute le script, le parent gère la communication
    pid_t childPid = fork();
    if (childPid == -1)
    {
        close(stdinPipe[0]);  close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        return buildHttpError(500, "Internal Server Error");
    }

    if (childPid == 0)
        runChildProcess(interpreter, scriptPath, argv, &envPointers[0], stdinPipe, stdoutPipe);

    // 5. parent : ferme les extrémités inutiles, ignore SIGPIPE (broken pipe si le script quitte tôt)
    close(stdinPipe[0]);
    close(stdoutPipe[1]);
    signal(SIGPIPE, SIG_IGN);

    // 6. envoie le body de la requête sur stdin du script
    if (!request.body.empty())
        writeFdFromString(stdinPipe[1], request.body);
    close(stdinPipe[1]);

    // 7. lit la sortie du script avec timeout
    bool        hasTimedOut = false;
    std::string cgiOutput   = readCgiOutputWithTimeout(stdoutPipe[0], childPid, hasTimedOut);

    if (hasTimedOut)
        return buildHttpError(504, "Gateway Timeout");

    // 8. attend la fin du process et vérifie son code de sortie
    int childExitStatus = 0;
    waitpid(childPid, &childExitStatus, 0);

    HttpResponse statusError = checkChildStatus(childExitStatus);
    if (statusError.status_code != 0)
        return statusError;

    // 9. parse la sortie CGI (headers + body) et retourne la réponse HTTP
    if (cgiOutput.empty())
        return buildHttpError(500, "Internal Server Error");

    return parseOutput(cgiOutput);
}
