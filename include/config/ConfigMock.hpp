#ifndef CONFIGMOCK_HPP
#define CONFIGMOCK_HPP

#include "Config.hpp"

// Fabrique un Config rempli EN DUR (aucun fichier lu, aucun parsing).
// But : permettre à l'équipe (core server, http layer) de bosser contre
// l'interface réelle pendant que le vrai parser est encore en chantier.
// Le jour où le parser est prêt, on remplace cet appel par le vrai parsing
// SANS toucher au code des autres (même interface = même getters).
Config createMockConfig();

#endif
