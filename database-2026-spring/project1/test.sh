#!/bin/bash

PASS=0
FAIL=0
DIR="$(dirname "$0")"
PROG="python3 $DIR/run.py"
P="DB_2020-17316> "

clean_db() {
    rm -rf "$DIR/DB"
}

normalize() {
    sed 's/[[:space:]]*$//' | sed 's/^-\{2,\}$/---/'
}

run_test() {
    local name="$1"
    local input="$2"
    local expected="$3"
    local actual
    actual=$(echo "$input" | $PROG 2>/dev/null)
    local norm_actual norm_expected
    norm_actual=$(echo "$actual" | normalize)
    norm_expected=$(echo "$expected" | normalize)
    if [ "$norm_actual" = "$norm_expected" ]; then
        echo "  PASS: $name"
        ((PASS++))
    else
        echo "  FAIL: $name"
        echo "    expected: $(echo "$norm_expected" | cat -v)"
        echo "    actual:   $(echo "$norm_actual" | cat -v)"
        ((FAIL++))
    fi
}

# Helper: run_test with clean DB beforehand.
run_test_clean() {
    clean_db
    run_test "$@"
}

# ============================================================
echo "=== 1. CREATE TABLE ==="
# ============================================================

run_test_clean "Create simple table" \
    "create table t (id int, primary key(id));" \
    "${P}'t' table is created"

run_test_clean "Create table no PK" \
    "create table t (id int, name char(10));" \
    "${P}'t' table is created"

run_test_clean "DuplicateColumnDefError" \
    "create table t (id int, id int);" \
    "${P}Create table has failed: column definition is duplicated"

run_test_clean "DuplicatePrimaryKeyDefError" \
    "create table t (id int, primary key(id), primary key(id));" \
    "${P}Create table has failed: primary key definition is duplicated"

run_test_clean "CharLengthError (zero)" \
    "create table t (name char(0));" \
    "${P}Char length should be over 0"

run_test_clean "CharLengthError (negative)" \
    "create table t (name char(-1));" \
    "${P}Char length should be over 0"

run_test_clean "PrimaryKeyColumnDefError" \
    "create table t (id int, primary key(nonexist));" \
    "${P}Create table has failed:cannot define non-existing column 'nonexist' as primary key"

run_test_clean "ForeignKeyColumnDefError" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (id int, foreign key(nonexist) references r(id));')" \
    "${P}'r' table is created
${P}Create table has failed: cannot define non-existing column 'nonexist' as foreign key"

run_test_clean "TableExistenceError" \
    "$(printf 'create table t (id int);\ncreate table t (id int);')" \
    "${P}'t' table is created
${P}Create table has failed: table with the same name already exists"

run_test_clean "ReferenceExistenceError (no table)" \
    "create table t (id int, foreign key(id) references nonexist(id));" \
    "${P}Create table has failed: foreign key references non existing table or column"

run_test_clean "ReferenceExistenceError (no column)" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(nonexist));')" \
    "${P}'r' table is created
${P}Create table has failed: foreign key references non existing table or column"

run_test_clean "ReferenceTypeError" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk char(10), foreign key(fk) references r(id));')" \
    "${P}'r' table is created
${P}Create table has failed: foreign key references wrong type"

run_test_clean "ReferenceTypeError (char length mismatch)" \
    "$(printf 'create table r (id char(10), primary key(id));\ncreate table t (fk char(5), foreign key(fk) references r(id));')" \
    "${P}'r' table is created
${P}Create table has failed: foreign key references wrong type"

run_test_clean "ReferenceNonPrimaryKeyError" \
    "$(printf 'create table r (id int, val int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(val));')" \
    "${P}'r' table is created
${P}Create table has failed: foreign key references non primary key column"

run_test_clean "ReferenceNonPrimaryKeyError (partial composite PK)" \
    "$(printf 'create table r (a int, b int, primary key(a, b));\ncreate table t (fk int, foreign key(fk) references r(a));')" \
    "${P}'r' table is created
${P}Create table has failed: foreign key references non primary key column"

run_test_clean "PK columns auto NOT NULL" \
    "$(printf 'create table t (id int, name char(5), primary key(id));\nexplain t;')" \
    "${P}'t' table is created
${P}
----------------------------------
column_name | type    | null | key
id          | int     | N    | PRI
name        | char(5) | Y    |
----------------------------------
2 rows in set"

run_test_clean "FK valid creation" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(id));')" \
    "${P}'r' table is created
${P}'t' table is created"

# ============================================================
echo ""
echo "=== 2. DROP TABLE ==="
# ============================================================

run_test_clean "Drop success" \
    "$(printf 'create table t (id int);\ndrop table t;')" \
    "${P}'t' table is created
${P}'t' table is dropped"

run_test_clean "Drop NoSuchTable" \
    "drop table t;" \
    "${P}Drop table has failed: no such table"

run_test_clean "DropReferencedTableError" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(id));\ndrop table r;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}Drop table has failed: 'r' is referenced by another table"

run_test_clean "Drop referenced after referencing dropped" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(id));\ndrop table t;\ndrop table r;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}'t' table is dropped
${P}'r' table is dropped"

# ============================================================
echo ""
echo "=== 3. EXPLAIN / DESCRIBE / DESC ==="
# ============================================================

run_test_clean "Explain with PK and FK" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (a char(10), b int, primary key(a), foreign key(b) references r(id));\nexplain t;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}
----------------------------------
column_name | type     | null | key
a           | char(10) | N    | PRI
b           | int      | Y    | FOR
----------------------------------
2 rows in set"

run_test_clean "Explain PRI/FOR column" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (a int, primary key(a), foreign key(a) references r(id));\nexplain t;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}
--------------------------------------
column_name | type | null | key
a           | int  | N    | PRI/FOR
--------------------------------------
1 row in set"

run_test_clean "Explain NoSuchTable" \
    "explain t;" \
    "${P}Explain has failed: no such table"

run_test_clean "Describe NoSuchTable" \
    "describe t;" \
    "${P}Describe has failed: no such table"

run_test_clean "Desc NoSuchTable" \
    "desc t;" \
    "${P}Desc has failed: no such table"

run_test_clean "Describe same as Explain" \
    "$(printf 'create table t (id int);\ndescribe t;')" \
    "${P}'t' table is created
${P}
------------------------------
column_name | type | null | key
id          | int  | Y    |
------------------------------
1 row in set"

run_test_clean "Desc same as Explain" \
    "$(printf 'create table t (id int);\ndesc t;')" \
    "${P}'t' table is created
${P}
------------------------------
column_name | type | null | key
id          | int  | Y    |
------------------------------
1 row in set"

# ============================================================
echo ""
echo "=== 4. SHOW TABLES ==="
# ============================================================

run_test_clean "Show tables empty" \
    "show tables;" \
    "${P}
------------------------
------------------------
0 rows in set"

run_test_clean "Show tables with data" \
    "$(printf 'create table alpha (id int);\ncreate table beta (id int);\nshow tables;')" \
    "${P}'alpha' table is created
${P}'beta' table is created
${P}
------------------------
alpha
beta
------------------------
2 rows in set"

# ============================================================
echo ""
echo "=== 5. RENAME TABLE ==="
# ============================================================

run_test_clean "Rename success" \
    "$(printf 'create table t (id int);\nrename table t to s;')" \
    "${P}'t' table is created
${P}'s' is renamed"

run_test_clean "Rename NoSuchTable" \
    "rename table t to s;" \
    "${P}Rename table has failed: no such table"

run_test_clean "RenameAlreadyExistError" \
    "$(printf 'create table t (id int);\ncreate table s (id int);\nrename table t to s;')" \
    "${P}'t' table is created
${P}'s' table is created
${P}Rename table has failed: there is already a table named 's'"

run_test_clean "Rename preserves data" \
    "$(printf 'create table t (id int);\ninsert into t values(42);\nrename table t to s;\nselect * from s;')" \
    "${P}'t' table is created
${P}The row is inserted
${P}'s' is renamed
${P}
----
ID
42
----
1 row in set"

run_test_clean "Rename updates FK refs" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(id));\nrename table r to s;\ndrop table s;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}'s' is renamed
${P}Drop table has failed: 's' is referenced by another table"

# ============================================================
echo ""
echo "=== 6. TRUNCATE TABLE ==="
# ============================================================

run_test_clean "Truncate success" \
    "$(printf 'create table t (id int);\ninsert into t values(1);\ntruncate table t;\nselect * from t;')" \
    "${P}'t' table is created
${P}The row is inserted
${P}'t' is truncated
${P}
--
ID
--
0 rows in set"

run_test_clean "Truncate NoSuchTable" \
    "truncate table t;" \
    "${P}Truncate table has failed: no such table"

run_test_clean "TruncateReferencedTableError" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(id));\ntruncate table r;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}Truncate table has failed: 'r' is referenced by another table"

# ============================================================
echo ""
echo "=== 7. INSERT ==="
# ============================================================

run_test_clean "Insert success" \
    "$(printf 'create table t (id int, name char(10));\ninsert into t values(1, '"'"'Alice'"'"');')" \
    "${P}'t' table is created
${P}The row is inserted"

run_test_clean "Insert NoSuchTable" \
    "insert into t values(1);" \
    "${P}Insert has failed: no such table"

run_test_clean "Insert with column list" \
    "$(printf 'create table t (id int, name char(10), age int);\ninsert into t (id, name) values(1, '"'"'Bob'"'"');\nselect * from t;')" \
    "${P}'t' table is created
${P}The row is inserted
${P}
-----------------
ID | NAME | AGE
1  | Bob  | null
-----------------
1 row in set"

run_test_clean "Insert char truncation" \
    "$(printf 'create table t (name char(3));\ninsert into t values('"'"'Hello'"'"');\nselect * from t;')" \
    "${P}'t' table is created
${P}The row is inserted
${P}
----
NAME
Hel
----
1 row in set"

# ============================================================
echo ""
echo "=== 8. SELECT ==="
# ============================================================

run_test_clean "Select empty table" \
    "$(printf 'create table t (id int, name char(5));\nselect * from t;')" \
    "${P}'t' table is created
${P}
-----------
ID | NAME
-----------
0 rows in set"

run_test_clean "SelectTableExistenceError" \
    "select * from t;" \
    "${P}Select has failed: 't' does not exist"

run_test_clean "Select with multiple rows" \
    "$(printf 'create table t (id int, val char(5));\ninsert into t values(1, '"'"'aaa'"'"');\ninsert into t values(2, '"'"'bbb'"'"');\ninsert into t values(3, '"'"'ccc'"'"');\nselect * from t;')" \
    "${P}'t' table is created
${P}The row is inserted
${P}The row is inserted
${P}The row is inserted
${P}
-----------
ID | VAL
1  | aaa
2  | bbb
3  | ccc
-----------
3 rows in set"

run_test_clean "Select null display" \
    "$(printf 'create table t (id int, name char(5));\ninsert into t (id) values(1);\nselect * from t;')" \
    "${P}'t' table is created
${P}The row is inserted
${P}
-----------
ID | NAME
1  | null
-----------
1 row in set"

# ============================================================
echo ""
echo "=== 9. Persistence ==="
# ============================================================

clean_db
# First run: create and insert.
echo "$(printf 'create table t (id int, name char(5));\ninsert into t values(1, '"'"'Hi'"'"');\nexit;')" | $PROG >/dev/null 2>&1
# Second run: data should persist.
run_test "Data persists across restarts" \
    "select * from t;" \
    "${P}
-----------
ID | NAME
1  | Hi
-----------
1 row in set"

# ============================================================
echo ""
echo "=== 10. Case insensitivity ==="
# ============================================================

run_test_clean "Table name case insensitive" \
    "$(printf 'CREATE TABLE MyTable (id int);\nshow tables;')" \
    "${P}'mytable' table is created
${P}
------------------------
mytable
------------------------
1 row in set"

run_test_clean "Column name case insensitive" \
    "$(printf 'create table t (MyCol int);\nexplain t;')" \
    "${P}'t' table is created
${P}
------------------------------
column_name | type | null | key
mycol       | int  | Y    |
------------------------------
1 row in set"

# ============================================================
echo ""
echo "=== 11. Syntax errors ==="
# ============================================================

run_test_clean "Syntax error" \
    "INVALID QUERY;" \
    "${P}Syntax error"

run_test_clean "Keyword as table name" \
    "CREATE TABLE select (id int);" \
    "${P}Syntax error"

run_test_clean "Error stops remaining in line" \
    "INVALID; create table t (id int);" \
    "${P}Syntax error"

# ============================================================
echo ""
echo "=== 12. Exit ==="
# ============================================================

run_test_clean "Exit stops processing" \
    "$(printf 'exit;\ncreate table t (id int);')" \
    ""

run_test_clean "Queries before exit run" \
    "$(printf 'create table t (id int);\nexit;')" \
    "${P}'t' table is created"

# ============================================================
echo ""
echo "=== 13. DELETE / UPDATE pass-through ==="
# ============================================================

run_test_clean "DELETE pass-through" \
    "DELETE FROM t;" \
    "${P}'DELETE' requested"

run_test_clean "UPDATE pass-through" \
    "UPDATE t SET x = 5;" \
    "${P}'UPDATE' requested"

# ============================================================
echo ""
echo "===================="
echo "Results: $PASS passed, $FAIL failed"
clean_db
[ "$FAIL" -eq 0 ] && exit 0 || exit 1

