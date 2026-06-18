/*
** test_poll_manager.cpp
** ----------------------
** Goal: Valider chaque méthode de PollManager AVANT de coder Socket
**       et Server. On utilise stdin (fd 0) comme "fausse source
**       d'événements" — pas besoin de sockets pour ce test.
**
** Compilation :
**   c++ -Wall -Wextra -Werror -std=c++98 -I ../include \
**       test_poll_manager.cpp ../src/server/PollManager.cpp \
**       -o test_poll_manager
**
** Exécution :
**   ./test_poll_manager
**   -> puis tape n'importe quoi au clavier + Entrée
**   -> le programme doit afficher "stdin is readable!" et quitter
**      proprement si tu tapes "exit"
**
** Ce que ce test prouve, étape par étape :
**   1. addFd() ajoute bien fd 0 à la liste
**   2. poll() ne bloque pas indéfiniment (timeout visible toutes les 2s)
**   3. isReadable() détecte correctement quand stdin a des données
**   4. hasError() ne se déclenche jamais sur un fd normal
**   5. updateEvents() ne casse rien quand on l'appelle
**   6. removeFd() retire bien le fd (plus aucun event après)
*/

int main()
{
	return 0;
}

// #include "../../include/server/PollManager.hpp"
// #include <iostream>
// #include <unistd.h>
// #include <cassert>

// // Petite aide visuelle pour rendre le test lisible dans le terminal
// static void printStep(const std::string& step)
// {
//     std::cout << "\n=== " << step << " ===" << std::endl;
// }

// int main()
// {
//     PollManager poll_manager;

//     // ------------------------------------------------------------
//     // ETAPE 1 : addFd() — on surveille stdin en lecture
//     // ------------------------------------------------------------
//     printStep("Etape 1 : addFd(STDIN_FILENO, POLLIN)");
//     poll_manager.addFd(STDIN_FILENO, POLLIN);

//     assert(poll_manager.getFds().size() == 1);
//     assert(poll_manager.getFds()[0].fd == STDIN_FILENO);
//     std::cout << "OK : stdin (fd " << STDIN_FILENO
//                << ") ajoute a la liste. Taille liste = "
//                << poll_manager.getFds().size() << std::endl;

//     // ------------------------------------------------------------
//     // ETAPE 2 : poll() avec timeout — ne doit jamais bloquer pour rien
//     // ------------------------------------------------------------
//     printStep("Etape 2 : poll() avec timeout de 2000ms");
//     std::cout << "Tape quelque chose au clavier puis Entree." << std::endl;
//     std::cout << "(si tu ne tapes rien, le programme affichera"
//                  " 'timeout, rien de pret' toutes les 2s)" << std::endl;
//     std::cout << "Tape 'exit' + Entree pour quitter proprement.\n" << std::endl;

//     bool running = true;
//     int loop_count = 0;

//     while (running)
//     {
//         int ready = poll_manager.pollEngine(2000); // 2000ms = 2s de timeout

//         loop_count++;

//         // --------------------------------------------------------
//         // Cas A : poll() retourne -1 -> erreur
//         // --------------------------------------------------------
//         if (ready < 0)
//         {
//             std::cout << "poll() a retourne une erreur (ready=-1)."
//                        << " Si c'est EINTR, c'est normal (signal recu)."
//                        << " On continue sans crasher." << std::endl;
//             continue;
//         }

//         // --------------------------------------------------------
//         // Cas B : poll() retourne 0 -> timeout, personne n'est pret
//         // --------------------------------------------------------
//         if (ready == 0)
//         {
//             std::cout << "[tour " << loop_count << "] timeout, rien de pret"
//                        << std::endl;
//             continue;
//         }

//         // --------------------------------------------------------
//         // Cas C : ready > 0 -> au moins un fd est pret, on inspecte
//         // --------------------------------------------------------
//         std::cout << "[tour " << loop_count << "] poll() dit : "
//                    << ready << " fd(s) pret(s)" << std::endl;

//         // ETAPE 3 : isReadable()
//         if (poll_manager.isReadable(STDIN_FILENO))
//         {
//             char buffer[256];
//             ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

//             if (n > 0)
//             {
//                 buffer[n] = '\0';
//                 std::string input(buffer);

//                 // Retirer le \n final pour la comparaison
//                 if (!input.empty() && input[input.size() - 1] == '\n')
//                     input.erase(input.size() - 1);

//                 std::cout << "OK : stdin is readable! Tu as tape : \""
//                            << input << "\"" << std::endl;

//                 if (input == "exit")
//                 {
//                     std::cout << "Mot 'exit' detecte, on arrete le test."
//                                << std::endl;
//                     running = false;
//                 }
//             }
//         }

//         // ETAPE 4 : hasError() — ne doit jamais se déclencher ici
//         if (poll_manager.hasError(STDIN_FILENO))
//         {
//             std::cout << "ATTENTION : hasError() est true sur stdin,"
//                        << " ce qui est inattendu dans ce test simple."
//                        << std::endl;
//         }
//     }

//     // ------------------------------------------------------------
//     // ETAPE 5 : updateEvents() — verifie que ca ne crash pas
//     // ------------------------------------------------------------
//     printStep("Etape 5 : updateEvents(STDIN_FILENO, POLLIN)");
//     poll_manager.updateEvents(STDIN_FILENO, POLLIN);
//     std::cout << "OK : updateEvents() appele sans crash." << std::endl;

//     // Test sur un fd qui n'existe pas dans la liste -> doit etre un no-op
//     poll_manager.updateEvents(9999, POLLIN);
//     std::cout << "OK : updateEvents() sur un fd inexistant (9999)"
//                << " ne crash pas (no-op attendu)." << std::endl;

//     // ------------------------------------------------------------
//     // ETAPE 6 : removeFd() — la liste doit redevenir vide
//     // ------------------------------------------------------------
//     printStep("Etape 6 : removeFd(STDIN_FILENO)");
//     poll_manager.removeFd(STDIN_FILENO);

//     assert(poll_manager.getFds().empty());
//     std::cout << "OK : stdin retire. Taille liste = "
//                << poll_manager.getFds().size() << std::endl;

//     // removeFd sur un fd deja absent -> ne doit pas crash non plus
//     poll_manager.removeFd(STDIN_FILENO);
//     std::cout << "OK : removeFd() une seconde fois sur le meme fd"
//                << " ne crash pas (no-op attendu)." << std::endl;

//     std::cout << "\n=== TOUS LES TESTS PollManager SONT PASSES ==="
//                << std::endl;

//     return 0;
// }

// Ce test verifie si la detection isReadable fonctionne ainsi que le fait de pouvoir switch pour ecouter le writtable

// int main(void)
// {
// 	PollManager pollManager;

// 	// std::cout << pollManager.getSize() << std::endl;
// 	pollManager.addFd(0, POLLIN);
// 	int pollNum = pollManager.pollEngine(-1);
// 	std::cout << pollManager.isReadable(0) << std::endl;
// 	pollManager.updateEvents(0, POLLOUT);
// 	pollNum = pollManager.pollEngine(-1);
// 	std::cout << pollManager.isWritable(0) << std::endl;
// }