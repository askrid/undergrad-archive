#!/bin/bash

PASS=0
FAIL=0
PROG="python3 $(dirname "$0")/run.py"
P="DB_2020-17316> "

run_test() {
    local name="$1"
    local input="$2"
    local expected="$3"
    local actual
    actual=$(echo "$input" | $PROG 2>/dev/null)
    if [ "$actual" = "$expected" ]; then
        echo "  PASS: $name"
        ((PASS++))
    else
        echo "  FAIL: $name"
        echo "    expected: $(echo "$expected" | cat -v)"
        echo "    actual:   $(echo "$actual" | cat -v)"
        ((FAIL++))
    fi
}

echo "=== 1. All query types ==="

run_test "CREATE TABLE" \
    "CREATE TABLE t (id int, PRIMARY KEY (id));" \
    "${P}'CREATE TABLE' requested"

run_test "DROP TABLE" \
    "DROP TABLE t;" \
    "${P}'DROP TABLE' requested"

run_test "EXPLAIN" \
    "EXPLAIN t;" \
    "${P}'EXPLAIN' requested"

run_test "DESCRIBE" \
    "DESCRIBE t;" \
    "${P}'DESCRIBE' requested"

run_test "DESC" \
    "DESC t;" \
    "${P}'DESC' requested"

run_test "SHOW TABLES" \
    "SHOW TABLES;" \
    "${P}'SHOW TABLES' requested"

run_test "RENAME TABLE" \
    "RENAME TABLE t TO s, t TO s;" \
    "${P}'RENAME TABLE' requested"

run_test "TRUNCATE TABLE" \
    "TRUNCATE TABLE t;" \
    "${P}'TRUNCATE TABLE' requested"

run_test "SELECT" \
    "SELECT * FROM t;" \
    "${P}'SELECT' requested"

run_test "SELECT with WHERE" \
    "SELECT a, b FROM t WHERE a >= 1 AND b != 2;" \
    "${P}'SELECT' requested"

run_test "SELECT with alias" \
    "SELECT t.col AS c FROM t AS tbl;" \
    "${P}'SELECT' requested"

run_test "SELECT with LIMIT/OFFSET" \
    "SELECT * FROM t LIMIT 10 OFFSET 5;" \
    "${P}'SELECT' requested"

run_test "INSERT" \
    "INSERT INTO t VALUES (1, \"hello\", 2024-01-01);" \
    "${P}'INSERT' requested"

run_test "INSERT with columns" \
    "INSERT INTO t (a, b) VALUES (1, \"x\");" \
    "${P}'INSERT' requested"

run_test "DELETE" \
    "DELETE FROM t;" \
    "${P}'DELETE' requested"

run_test "DELETE with WHERE" \
    "DELETE FROM t WHERE x = 1;" \
    "${P}'DELETE' requested"

run_test "UPDATE" \
    "UPDATE t SET x = 5;" \
    "${P}'UPDATE' requested"

run_test "UPDATE with WHERE" \
    "UPDATE t SET x = 5 WHERE y = 1;" \
    "${P}'UPDATE' requested"

echo ""
echo "=== 2. Sequence of queries in one line ==="

run_test "Two queries on one line" \
    "SELECT * FROM t; DELETE FROM t;" \
    "${P}'SELECT' requested
${P}'DELETE' requested"

run_test "Three queries on one line" \
    "SELECT * FROM t; DROP TABLE t; SHOW TABLES;" \
    "${P}'SELECT' requested
${P}'DROP TABLE' requested
${P}'SHOW TABLES' requested"

echo ""
echo "=== 3. Query split across multiple lines ==="

run_test "CREATE TABLE multiline" \
    "$(printf 'CREATE TABLE t\n(id int,\nPRIMARY KEY (id));')" \
    "${P}'CREATE TABLE' requested"

run_test "SELECT multiline" \
    "$(printf 'SELECT *\nFROM t\nWHERE a = 1;')" \
    "${P}'SELECT' requested"

echo ""
echo "=== 4. Multiple queries split across multiple lines ==="

run_test "Two queries across lines" \
    "$(printf 'SELECT *\nFROM t; DELETE\nFROM t;')" \
    "${P}'SELECT' requested
${P}'DELETE' requested"

run_test "Three queries across lines" \
    "$(printf 'CREATE TABLE t\n(id int); DROP\nTABLE t; SHOW\nTABLES;')" \
    "${P}'CREATE TABLE' requested
${P}'DROP TABLE' requested
${P}'SHOW TABLES' requested"

echo ""
echo "=== 5. Keywords as identifiers should fail ==="

run_test "Keyword as table name" \
    "CREATE TABLE select (id int);" \
    "${P}Syntax error"

run_test "Keyword as column name" \
    "SELECT from FROM t;" \
    "${P}Syntax error"

run_test "Keyword as alias" \
    "SELECT a AS where FROM t;" \
    "${P}Syntax error"

echo ""
echo "=== 6. Syntax error stops remaining queries on same line ==="

run_test "Error skips rest of line" \
    "INVALID QUERY; SELECT * FROM t;" \
    "${P}Syntax error"

run_test "Valid then error skips rest" \
    "SHOW TABLES; INVALID; SELECT * FROM t;" \
    "${P}'SHOW TABLES' requested
${P}Syntax error"

run_test "Continues on next line after error" \
    "$(printf 'INVALID QUERY; SELECT * FROM t;\nSHOW TABLES;')" \
    "${P}Syntax error
${P}'SHOW TABLES' requested"

echo ""
echo "=== 7. Exit terminates program ==="

run_test "exit stops processing" \
    "$(printf 'exit;\nSHOW TABLES;')" \
    ""

run_test "Queries before exit still run" \
    "$(printf 'SHOW TABLES;\nexit;')" \
    "${P}'SHOW TABLES' requested"

run_test "exit mid-line stops processing" \
    "SHOW TABLES; exit; SELECT * FROM t;" \
    "${P}'SHOW TABLES' requested"

echo ""
echo "===================="
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ] && exit 0 || exit 1

