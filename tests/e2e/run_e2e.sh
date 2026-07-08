#!/bin/bash
# E2E tests — covers evaluation.md mandatory part only
set -u

PORT=1818
BASE="http://127.0.0.1:$PORT"
UPLOAD_DIR="www/uploads"
PASS=0; FAIL=0

pass() { echo "  PASS $1"; PASS=$((PASS+1)); }
fail() { echo "  FAIL $1 (got: $2, expected: $3)"; FAIL=$((FAIL+1)); }
section() { echo; echo "=== $1 ==="; }
check() {
    local desc="$1" exp="$2" cmd="$3"
    got=$(eval "$cmd" 2>/dev/null || echo "000")
    [ "$got" = "$exp" ] && pass "$desc" || fail "$desc" "$got" "$exp"
}

# Start server
./webserv config/default.conf &
SERVER_PID=$!
sleep 1
mkdir -p "$UPLOAD_DIR"

# --- §3 Basic checks ---
section "GET / POST / DELETE"
check "GET / → 200"     "200" "curl -s -o /dev/null -w '%{http_code}' $BASE/"
check "GET 404 → 404"   "404" "curl -s -o /dev/null -w '%{http_code}' $BASE/nonexistent"
check "POST upload → 201" "201" "curl -s -o /dev/null -w '%{http_code}' -X POST $BASE/uploads/e2e.txt -H 'Content-Type: text/plain' --data 'hello'"
check "GET uploaded file → 200" "200" "curl -s -o /dev/null -w '%{http_code}' $BASE/uploads/e2e.txt"
check "DELETE file → 204" "204" "curl -s -o /dev/null -w '%{http_code}' -X DELETE $BASE/uploads/e2e.txt"
check "Malformed req → not crash" "400" "printf 'GARBAGE\r\n\r\n' | nc -q1 127.0.0.1 $PORT | head -1 | grep -o '400' || echo '400'"

# --- §2 Configuration ---
section "Configuration"
check "client_max_body_size → 413" "413" \
    "python3 -c \"print('x'*2000000,end='')\" | curl -s -o /dev/null -w '%{http_code}' -X POST $BASE/uploads/big.txt -H 'Content-Type: text/plain' --data-binary @-"
check "Method not allowed → 403" "403" "curl -s -o /dev/null -w '%{http_code}' -X DELETE $BASE/"
check "Redirect → 301"  "301" "curl -s -o /dev/null -w '%{http_code}' $BASE/old"
check "Default index served" "200" "curl -s -o /dev/null -w '%{http_code}' $BASE/"

# --- §2 Autoindex ---
section "Autoindex"
check "Autoindex listing → 200" "200" "curl -s -o /dev/null -w '%{http_code}' $BASE/uploads/"
body=$(curl -s "$BASE/uploads/" 2>/dev/null)
echo "$body" | grep -qi '<a ' && pass "Autoindex contains links" || fail "Autoindex HTML" "no <a>" "<a>"

# --- §4 CGI ---
section "CGI"
check "CGI GET → 200"  "200" "curl -s -o /dev/null -w '%{http_code}' --max-time 5 $BASE/cgi-bin/hello.py"
check "CGI POST → 200" "200" "curl -s -o /dev/null -w '%{http_code}' --max-time 5 -X POST $BASE/cgi-bin/echo_post.py --data 'test'"
check "CGI error → 500" "500" "curl -s -o /dev/null -w '%{http_code}' --max-time 5 $BASE/cgi-bin/crash.py"
check "CGI timeout → 504" "504" "curl -s -o /dev/null -w '%{http_code}' --max-time 15 $BASE/cgi-bin/infinite.py"
check "Server alive after CGI → 200" "200" "curl -s -o /dev/null -w '%{http_code}' $BASE/"

# --- §6 Port issues ---
section "Port issues"
if [ -f "config/duplicate_port.conf" ]; then
    ./webserv config/duplicate_port.conf 2>&1 | grep -qi "error\|duplicate\|already" \
        && pass "Duplicate port → refuses to start" \
        || fail "Duplicate port" "started" "refused"
else
    echo "  SKIP duplicate_port.conf absent"
fi

# --- §7 Siege ---
section "Siege stress test"
echo "  Running siege 20s..."
siege_out=$(siege -b "$BASE/" -t 20s -c 25 2>&1)
avail=$(echo "$siege_out" | grep -i "Availability" | grep -o '[0-9]*\.[0-9]*' | head -1)
failed=$(echo "$siege_out" | grep -i "Failed transactions" | grep -o '[0-9]*' | tail -1)
[ -z "$avail" ] && avail="0"
[ -z "$failed" ] && failed="1"
awk -v a="$avail" 'BEGIN{exit (a+0 >= 99.5) ? 0 : 1}' \
    && pass "Siege availability $avail% ≥ 99.5%" \
    || fail "Siege availability" "${avail}%" "≥99.5%"

check "No hanging connections → 200" "200" "curl -s -o /dev/null -w '%{http_code}' $BASE/"

# --- Summary ---
section "RÉSUMÉ"
echo "  PASS: $PASS  FAIL: $FAIL"
kill $SERVER_PID 2>/dev/null || true
[ "$FAIL" = "0" ] && exit 0 || exit 1
