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
------------------------------
column_name | type | null | key
id          | int  | Y    |
------------------------------
1 row in set"

run_test_clean "Desc same as Explain" \
    "$(printf 'create table t (id int);\ndesc t;')" \
    "${P}'t' table is created
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
    "------------------------
------------------------
0 rows in set"

run_test_clean "Show tables with data" \
    "$(printf 'create table alpha (id int);\ncreate table beta (id int);\nshow tables;')" \
    "${P}'alpha' table is created
${P}'beta' table is created
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
${P}1 row inserted
${P}'s' is renamed
----
id
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
${P}1 row inserted
${P}'t' is truncated
--
id
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
${P}1 row inserted"

run_test_clean "Insert NoSuchTable" \
    "insert into t values(1);" \
    "${P}Insert has failed: no such table"

run_test_clean "Insert with column list" \
    "$(printf 'create table t (id int, name char(10), age int);\ninsert into t (id, name) values(1, '"'"'Bob'"'"');\nselect * from t;')" \
    "${P}'t' table is created
${P}1 row inserted
-----------------
id | name | age
1  | Bob  | null
-----------------
1 row in set"

run_test_clean "Insert char truncation" \
    "$(printf 'create table t (name char(3));\ninsert into t values('"'"'Hello'"'"');\nselect * from t;')" \
    "${P}'t' table is created
${P}1 row inserted
----
name
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
-----------
id | name
-----------
0 rows in set"

run_test_clean "SelectTableExistenceError" \
    "select * from t;" \
    "${P}Select has failed: 't' does not exist"

run_test_clean "Select with multiple rows" \
    "$(printf 'create table t (id int, val char(5));\ninsert into t values(1, '"'"'aaa'"'"');\ninsert into t values(2, '"'"'bbb'"'"');\ninsert into t values(3, '"'"'ccc'"'"');\nselect * from t;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
-----------
id | val
1  | aaa
2  | bbb
3  | ccc
-----------
3 rows in set"

run_test_clean "Select null display" \
    "$(printf 'create table t (id int, name char(5));\ninsert into t (id) values(1);\nselect * from t;')" \
    "${P}'t' table is created
${P}1 row inserted
-----------
id | name
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
"-----------
id | name
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
------------------------
mytable
------------------------
1 row in set"

run_test_clean "Column name case insensitive" \
    "$(printf 'create table t (MyCol int);\nexplain t;')" \
    "${P}'t' table is created
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
echo "=== 13. INSERT errors (1-3) ==="
# ============================================================

run_test_clean "Insert NoSuchTable" \
    "insert into nope values(1);" \
    "${P}Insert has failed: no such table"

run_test_clean "Insert column does not exist" \
    "$(printf 'create table t (id int);\ninsert into t (bogus) values(1);')" \
    "${P}'t' table is created
${P}Insert has failed: 'bogus' does not exist"

run_test_clean "Insert type mismatch (length)" \
    "$(printf 'create table t (a int, b int);\ninsert into t values(1);')" \
    "${P}'t' table is created
${P}Insert has failed: types are not matched"

run_test_clean "Insert type mismatch (type)" \
    "$(printf 'create table t (a int);\ninsert into t values('"'"'oops'"'"');')" \
    "${P}'t' table is created
${P}Insert has failed: types are not matched"

run_test_clean "Insert null into NOT NULL (PK)" \
    "$(printf 'create table t (id int, primary key(id));\ninsert into t (id) values(null);')" \
    "${P}'t' table is created
${P}Insert has failed: 'id' is not nullable"

# ============================================================
echo ""
echo "=== 14. DELETE (1-3) ==="
# ============================================================

run_test_clean "Delete no such table" \
    "delete from nope;" \
    "${P}Delete has failed: no such table"

run_test_clean "Delete all (no WHERE)" \
    "$(printf 'create table t (id int);\ninsert into t values(1);\ninsert into t values(2);\ndelete from t;\nselect * from t;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}'2' row(s) deleted
--
id
--
0 rows in set"

run_test_clean "Delete with WHERE" \
    "$(printf 'create table t (id int, name char(5));\ninsert into t values(1, '"'"'a'"'"');\ninsert into t values(2, '"'"'b'"'"');\ndelete from t where id = 1;\nselect * from t;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}'1' row(s) deleted
-----------
id | name
2  | b
-----------
1 row in set"

run_test_clean "Delete referential integrity blocked" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(id));\ninsert into r values(1);\ninsert into t values(1);\ndelete from r;\nselect * from r;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}'1' row(s) are not deleted due to referential integrity
----
id
1
----
1 row in set"

# ============================================================
echo ""
echo "=== 15. SELECT projection + WHERE (1-3) ==="
# ============================================================

run_test_clean "Select single column" \
    "$(printf 'create table t (id int, name char(5));\ninsert into t values(1, '"'"'a'"'"');\nselect id from t;')" \
    "${P}'t' table is created
${P}1 row inserted
--
id
1
--
1 row in set"

run_test_clean "Select WHERE equality" \
    "$(printf 'create table t (id int);\ninsert into t values(1);\ninsert into t values(2);\nselect * from t where id = 2;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
--
id
2
--
1 row in set"

run_test_clean "Select WHERE incomparable" \
    "$(printf 'create table t (a int);\ninsert into t values(1);\nselect * from t where a = '"'"'x'"'"';')" \
    "${P}'t' table is created
${P}1 row inserted
${P}Trying to compare incomparable columns or values"

run_test_clean "Select WHERE column not exist" \
    "$(printf 'create table t (a int);\nselect * from t where nope = 1;')" \
    "${P}'t' table is created
${P}where clause trying to reference non existing column"

run_test_clean "Select WHERE table not specified" \
    "$(printf 'create table t (a int);\nselect * from t where other.a = 1;')" \
    "${P}'t' table is created
${P}where clause trying to reference tables which are not specified"

run_test_clean "Select WHERE IS NULL" \
    "$(printf 'create table t (id int, name char(5));\ninsert into t (id) values(1);\nselect * from t where name is null;')" \
    "${P}'t' table is created
${P}1 row inserted
-----------
id | name
1  | null
-----------
1 row in set"

# ============================================================
echo ""
echo "=== 16. SELECT JOIN (1-3) ==="
# ============================================================

run_test_clean "Inner join two tables" \
    "$(printf 'create table r (id int, primary key(id));\ncreate table t (fk int, foreign key(fk) references r(id));\ninsert into r values(1);\ninsert into r values(2);\ninsert into t values(1);\nselect r.id, t.fk from r join t on r.id = t.fk;')" \
    "${P}'r' table is created
${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
-----------
r.id | t.fk
1    | 1
-----------
1 row in set"

run_test_clean "Join ambiguous unqualified column" \
    "$(printf 'create table a (x int);\ncreate table b (x int);\ninsert into a values(1);\ninsert into b values(1);\nselect x from a join b on a.x = b.x;')" \
    "${P}'a' table is created
${P}'b' table is created
${P}1 row inserted
${P}1 row inserted
${P}Select has failed: fail to resolve 'x'"

# ============================================================
echo ""
echo "=== 17. ORDER BY / LIMIT / OFFSET (1-3) ==="
# ============================================================

run_test_clean "Order by asc" \
    "$(printf 'create table t (id int);\ninsert into t values(3);\ninsert into t values(1);\ninsert into t values(2);\nselect * from t order by id asc;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
--
id
1
2
3
--
3 rows in set"

run_test_clean "Order by desc" \
    "$(printf 'create table t (id int);\ninsert into t values(1);\ninsert into t values(3);\ninsert into t values(2);\nselect * from t order by id desc;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
--
id
3
2
1
--
3 rows in set"

run_test_clean "Limit and offset" \
    "$(printf 'create table t (id int);\ninsert into t values(1);\ninsert into t values(2);\ninsert into t values(3);\ninsert into t values(4);\nselect * from t order by id asc limit 2 offset 1;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
--
id
2
3
--
2 rows in set"

run_test_clean "Negative limit error" \
    "$(printf 'create table t (id int);\nselect * from t limit -1;')" \
    "${P}'t' table is created
${P}Select has failed: LIMIT/OFFSET clause should be a non-negative integer"

# ============================================================
echo ""
echo "=== 18. GROUP BY + aggregates (optional) ==="
# ============================================================

run_test_clean "MAX with GROUP BY" \
    "$(printf 'create table t (g char(1), v int);\ninsert into t values('"'"'a'"'"', 5);\ninsert into t values('"'"'a'"'"', 9);\ninsert into t values('"'"'b'"'"', 3);\nselect g, max(v) from t group by g order by g asc;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
----------
g | max(v)
a | 9
b | 3
----------
2 rows in set"

run_test_clean "MIN with GROUP BY" \
    "$(printf 'create table t (g char(1), v int);\ninsert into t values('"'"'a'"'"', 5);\ninsert into t values('"'"'a'"'"', 9);\ninsert into t values('"'"'b'"'"', 3);\nselect g, min(v) from t group by g order by g asc;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
----------
g | min(v)
a | 5
b | 3
----------
2 rows in set"

run_test_clean "SUM with GROUP BY" \
    "$(printf 'create table t (g char(1), v int);\ninsert into t values('"'"'a'"'"', 5);\ninsert into t values('"'"'a'"'"', 9);\ninsert into t values('"'"'b'"'"', 3);\nselect g, sum(v) from t group by g order by g asc;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
${P}1 row inserted
----------
g | sum(v)
a | 14
b | 3
----------
2 rows in set"

run_test_clean "Aggregate without GROUP BY (single group)" \
    "$(printf 'create table t (v int);\ninsert into t values(10);\ninsert into t values(20);\nselect max(v) from t;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}1 row inserted
------
max(v)
20
------
1 row in set"

run_test_clean "SelectColumnNotGrouped" \
    "$(printf 'create table t (a int, b int);\ninsert into t values(1, 10);\nselect a, b from t group by a;')" \
    "${P}'t' table is created
${P}1 row inserted
${P}Select has failed: column 'b' must either be included in the GROUP BY clause or be used in an aggregate function"

run_test_clean "MAX returns null on empty / all-null" \
    "$(printf 'create table t (v int);\ninsert into t values(null);\nselect max(v) from t;')" \
    "${P}'t' table is created
${P}1 row inserted
------
max(v)
null
------
1 row in set"

run_test_clean "SUM returns 0 on empty / all-null" \
    "$(printf 'create table t (v int);\ninsert into t values(null);\nselect sum(v) from t;')" \
    "${P}'t' table is created
${P}1 row inserted
------
sum(v)
0
------
1 row in set"

run_test_clean "GROUP BY column not exist" \
    "$(printf 'create table t (a int);\nselect a from t group by nope;')" \
    "${P}'t' table is created
${P}group by clause trying to reference non existing column"

# ============================================================
echo ""
echo "===================="
echo "Results: $PASS passed, $FAIL failed"
clean_db
[ "$FAIL" -eq 0 ] && exit 0 || exit 1

