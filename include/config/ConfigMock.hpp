#ifndef CONFIGMOCK_HPP
#define CONFIGMOCK_HPP

#include "Config.hpp"

// Construit un Config aux valeurs codées en dur (aucun fichier lu, aucun parsing).
// Objectif : fournir l'interface réelle aux modules core server et http layer
// pendant que le parser est en cours de développement.
// Une fois le parser disponible, cet appel sera remplacé par le parsing réel
// sans modifier le code consommateur (interface identique).
Config createMockConfig();

#endif
