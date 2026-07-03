#!/bin/sh
# Teste tous les fichiers de config du repo :
#   - config/default.conf et config/valid/*.conf doivent être ACCEPTÉS
#   - config/invalid/*.conf doivent être REJETÉS (avec un message, sans crash)
# Usage : sh tests/config/run_config_tests.sh   (depuis la racine du repo)

set -u

BIN=/tmp/webserv_check_config
PASSED=0
FAILED=0

echo "--- Compilation du checker ---"
c++ -Wall -Wextra -Werror -std=c++98 -I include \
    tests/config/check_config.cpp \
    src/config/ConfigParser.cpp src/config/ConfigValidator.cpp \
    src/config/Tokenizer.cpp src/config/Config.cpp \
    src/config/ServerConfig.cpp src/config/LocationConfig.cpp \
    -o "$BIN" || exit 1

echo ""
echo "--- Configs VALIDES (doivent passer) ---"
for f in config/default.conf config/valid/*.conf; do
    output=$("$BIN" "$f" 2>&1)
    if [ $? -eq 0 ]; then
        echo "[OK] $f"
        PASSED=$((PASSED + 1))
    else
        echo "[KO] $f aurait dû être accepté :"
        echo "     $output"
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "--- Configs INVALIDES (doivent être rejetées proprement) ---"
for f in config/invalid/*.conf; do
    output=$("$BIN" "$f" 2>&1)
    status=$?
    if [ $status -eq 1 ]; then
        echo "[OK] $f -> ${output#REJECTED: }"
        PASSED=$((PASSED + 1))
    elif [ $status -eq 0 ]; then
        echo "[KO] $f aurait dû être rejeté"
        FAILED=$((FAILED + 1))
    else
        # status > 1 = crash (segfault, abort...) : interdit par le sujet.
        echo "[KO] $f a fait CRASHER le checker (exit $status)"
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "Resultat : $PASSED OK, $FAILED KO"
[ "$FAILED" -eq 0 ]
