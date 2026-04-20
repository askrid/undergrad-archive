import json
import lmdb
import sys
import os
from concurrent.futures import Future
from threading import Thread
from typing import Any, cast
from lark import Lark, Token, Tree, UnexpectedInput, Transformer

PROMPT = "DB_2020-17316> "
GRAMMAR_FILE = "grammar.lark"
DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "DB")
DEBUG = False

parser: Lark | None = None
parser_future: Future[Lark] = Future()
db: "Database|None" = None
transformer: "SQLTransformer|None" = None


class Database:
    """LMDB-backed persistent storage for table schemas and row data."""

    def __init__(self, path: str = DB_PATH):
        os.makedirs(path, exist_ok=True)
        self.env: lmdb.Environment = lmdb.open(
            path, map_size=10**9, subdir=True, create=True
        )

    def close(self) -> None:
        self.env.close()

    def _get(self, key: str) -> Any:
        with self.env.begin() as txn:
            v = txn.get(key.encode())
            return json.loads(v) if v else None

    def _put(self, key: str, val: Any) -> None:
        with self.env.begin(write=True) as txn:
            txn.put(key.encode(), json.dumps(val, ensure_ascii=False).encode())

    def _del(self, key: str) -> None:
        with self.env.begin(write=True) as txn:
            txn.delete(key.encode())

    def get_tables(self) -> list[str]:
        return self._get("meta:tables") or []

    def has_table(self, name: str) -> bool:
        return name in self.get_tables()

    def get_schema(self, name: str) -> dict[str, Any]:
        return self._get(f"schema:{name}")

    def create_table(self, name: str, schema: dict[str, Any]) -> None:
        tl = self.get_tables()
        tl.append(name)
        self._put("meta:tables", tl)
        self._put(f"schema:{name}", schema)
        self._put(f"data:{name}", [])

    def drop_table(self, name: str) -> None:
        tl = self.get_tables()
        tl.remove(name)
        self._put("meta:tables", tl)
        self._del(f"schema:{name}")
        self._del(f"data:{name}")

    def rename_table(self, old: str, new: str) -> None:
        tl = self.get_tables()
        tl[tl.index(old)] = new
        self._put("meta:tables", tl)
        schema = self.get_schema(old)
        data = self._get(f"data:{old}") or []
        self._put(f"schema:{new}", schema)
        self._put(f"data:{new}", data)
        self._del(f"schema:{old}")
        self._del(f"data:{old}")
        # Update FK references in other tables pointing to old name.
        for t in tl:
            s = self.get_schema(t)
            if not s:
                continue
            changed = False
            for fk in s.get("foreign_keys", []):
                if fk["ref_table"] == old:
                    fk["ref_table"] = new
                    changed = True
            if changed:
                self._put(f"schema:{t}", s)

    def get_rows(self, name: str) -> list[list[Any]]:
        return self._get(f"data:{name}") or []

    def add_row(self, name: str, row: list[Any]) -> None:
        rows = self.get_rows(name)
        rows.append(row)
        self._put(f"data:{name}", rows)

    def clear_rows(self, name: str) -> None:
        self._put(f"data:{name}", [])

    def is_referenced(self, name: str) -> bool:
        """Check if any other existing table has FK referencing this table."""
        for t in self.get_tables():
            if t == name:
                continue
            s = self.get_schema(t)
            if s:
                for fk in s.get("foreign_keys", []):
                    if fk["ref_table"] == name:
                        return True
        return False


def _fmt_table(headers: list[str], rows: list[list[str]]) -> str:
    """Format tabular data with dashes, headers, rows, and row count."""
    all_data: list[list[str]] = [headers] + rows
    ncols = len(headers)
    widths = [max(len(str(r[i])) for r in all_data) for i in range(ncols)]

    def frow(r: list[str]) -> str:
        return " | ".join(str(r[i]).ljust(widths[i]) for i in range(ncols))

    tw = sum(widths) + 3 * (ncols - 1)
    dash = "-" * tw
    lines = [dash, frow(headers)]
    for r in rows:
        lines.append(frow(r))
    lines.append(dash)
    n = len(rows)
    lines.append(f"{n} row{'s' if n != 1 else ''} in set")
    return "\n".join(lines)


def _strs(items: list[Any]) -> list[str]:
    """Extract plain strings (transformed rule results) from items,
    filtering out Token objects (which are str subclass)."""
    return [i for i in items if isinstance(i, str) and not isinstance(i, Token)]


class SQLTransformer(Transformer):  # type: ignore[type-arg]
    """
    Transforms parsed SQL tree and executes queries against the LMDB-backed
    database. The result is either a message string or None (EXIT).
    """

    type Result = str | None

    def __init__(self, database: Database):
        super().__init__()
        self.db = database

    # ---- Low-level data extraction ----

    def table_name(self, items: list[Token]) -> str:
        return items[0].lower()

    def column_name(self, items: list[Token]) -> str:
        return str(items[0]).lower()

    def data_type(self, items: list[Token]) -> list[str | int]:
        t = items[0]
        if t.type == "TYPE_INT":
            return ["int"]
        elif t.type == "TYPE_CHAR":
            return ["char", int(str(items[2]))]
        elif t.type == "TYPE_DATE":
            return ["date"]
        raise ValueError(f"Unknown data type: {t}")

    def column_definition(self, items: list[Any]) -> dict[str, Any]:
        parts = [i for i in items if not isinstance(i, Token)]
        not_null = any(isinstance(i, Token) and i.type == "NOT" for i in items)
        return {"k": "col", "name": parts[0], "type": parts[1], "nn": not_null}

    def column_name_list(self, items: list[Any]) -> list[str]:
        return [i for i in items if not isinstance(i, Token)]

    def primary_key_constraint(self, items: list[Any]) -> dict[str, Any]:
        return {"k": "pk", "cols": next(i for i in items if isinstance(i, list))}

    def referential_constraint(self, items: list[Any]) -> dict[str, Any]:
        lists = [i for i in items if isinstance(i, list)]
        strs = _strs(items)
        return {"k": "fk", "cols": lists[0], "ref": strs[0], "rcols": lists[1]}

    def table_constraint_definition(self, items: list[Any]) -> dict[str, Any]:
        return items[0]

    def table_element(self, items: list[Any]) -> dict[str, Any]:
        return items[0]

    def table_element_list(self, items: list[Any]) -> list[dict[str, Any]]:
        return [i for i in items if isinstance(i, dict)]

    def value(self, items: list[Token]) -> int | str | None:
        t = items[0]
        if t.type == "INT":
            return int(str(t))
        elif t.type == "STR":
            return str(t)[1:-1]
        elif t.type == "DATE":
            return str(t)
        elif t.type == "NULL":
            return None
        raise ValueError(f"Unknown value type: {t}")

    def value_list(self, items: list[Any]) -> list[Any]:
        result: list[Any] = []
        for i in items:
            if isinstance(i, Token):
                continue
            result.append(i)
        return result

    # ---- SELECT helpers ----

    def select_list(self, items: list[Any]) -> str:
        return "*"

    def referred_table(self, items: list[Any]) -> str:
        return _strs(items)[0]

    def table_reference_list(self, items: list[Any]) -> list[str]:
        return _strs(items)

    def from_clause(self, items: list[Any]) -> list[str]:
        return next(i for i in items if isinstance(i, list))

    def table_expression(self, items: list[Any]) -> list[str]:
        for i in items:
            if isinstance(i, list):
                return i
        return []

    # ---- DDL: CREATE TABLE ----

    def create_table_query(self, items: list[Any]) -> str:
        parts = [i for i in items if not isinstance(i, Token)]
        tname: str = parts[0]
        elems: list[dict[str, Any]] = parts[1]

        cols = [e for e in elems if e["k"] == "col"]
        pks = [e for e in elems if e["k"] == "pk"]
        fks = [e for e in elems if e["k"] == "fk"]
        cnames = [c["name"] for c in cols]

        # CharLengthError
        for c in cols:
            if c["type"][0] == "char" and c["type"][1] <= 0:
                return "Char length should be over 0"

        # DuplicateColumnDefError
        if len(cnames) != len(set(cnames)):
            return "Create table has failed: column definition is duplicated"

        # DuplicatePrimaryKeyDefError
        if len(pks) > 1:
            return "Create table has failed: primary key definition is duplicated"

        pk_cols: list[str] = pks[0]["cols"] if pks else []

        # PrimaryKeyColumnDefError
        for pc in pk_cols:
            if pc not in cnames:
                return (
                    f"Create table has failed:"
                    f"cannot define non-existing column '{pc}' as primary key"
                )

        # ForeignKeyColumnDefError
        for fk in fks:
            for fc in fk["cols"]:
                if fc not in cnames:
                    return (
                        f"Create table has failed: "
                        f"cannot define non-existing column '{fc}' as foreign key"
                    )

        # TableExistenceError
        if self.db.has_table(tname):
            return "Create table has failed: table with the same name already exists"

        # ReferenceExistenceError
        for fk in fks:
            if not self.db.has_table(fk["ref"]):
                return (
                    "Create table has failed: "
                    "foreign key references non existing table or column"
                )
            rs = self.db.get_schema(fk["ref"])
            rc_names = [c["name"] for c in rs["columns"]]
            for rc in fk["rcols"]:
                if rc not in rc_names:
                    return (
                        "Create table has failed: "
                        "foreign key references non existing table or column"
                    )

        # ReferenceTypeError
        col_types = {c["name"]: c["type"] for c in cols}
        for fk in fks:
            rs = self.db.get_schema(fk["ref"])
            rt = {c["name"]: c["type"] for c in rs["columns"]}
            for fc, rc in zip(fk["cols"], fk["rcols"]):
                if col_types[fc] != rt[rc]:
                    return (
                        "Create table has failed: " "foreign key references wrong type"
                    )

        # ReferenceNonPrimaryKeyError
        for fk in fks:
            rs = self.db.get_schema(fk["ref"])
            rpk: list[str] = rs.get("primary_key", [])
            if sorted(fk["rcols"]) != sorted(rpk):
                return (
                    "Create table has failed: "
                    "foreign key references non primary key column"
                )

        # All checks passed. PK columns become NOT NULL.
        for c in cols:
            if c["name"] in pk_cols:
                c["nn"] = True

        schema: dict[str, Any] = {
            "columns": [
                {"name": c["name"], "type": c["type"], "not_null": c["nn"]}
                for c in cols
            ],
            "primary_key": pk_cols,
            "foreign_keys": [
                {"columns": f["cols"], "ref_table": f["ref"], "ref_columns": f["rcols"]}
                for f in fks
            ],
        }
        self.db.create_table(tname, schema)
        return f"'{tname}' table is created"

    # ---- DDL: DROP TABLE ----

    def drop_table_query(self, items: list[Any]) -> str:
        tname: str = _strs(items)[0]
        if not self.db.has_table(tname):
            return "Drop table has failed: no such table"
        if self.db.is_referenced(tname):
            return f"Drop table has failed: '{tname}' is referenced by another table"
        self.db.drop_table(tname)
        return f"'{tname}' table is dropped"

    # ---- DDL: EXPLAIN / DESCRIBE / DESC ----

    def _explain(self, tname: str) -> str:
        s = self.db.get_schema(tname)
        pk = set(s.get("primary_key", []))
        fk_set: set[str] = set()
        for fk in s.get("foreign_keys", []):
            fk_set.update(fk["columns"])

        hdrs = ["column_name", "type", "null", "key"]
        rows: list[list[str]] = []
        for c in s["columns"]:
            tp = c["type"][0]
            if tp == "char":
                tp = f"char({c['type'][1]})"
            nl = "N" if c["not_null"] else "Y"
            kp: list[str] = []
            if c["name"] in pk:
                kp.append("PRI")
            if c["name"] in fk_set:
                kp.append("FOR")
            rows.append([c["name"], tp, nl, "/".join(kp)])
        return _fmt_table(hdrs, rows)

    def explain_query(self, items: list[Any]) -> str:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return "Explain has failed: no such table"
        return self._explain(t)

    def describe_query(self, items: list[Any]) -> str:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return "Describe has failed: no such table"
        return self._explain(t)

    def desc_query(self, items: list[Any]) -> str:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return "Desc has failed: no such table"
        return self._explain(t)

    # ---- DDL: SHOW TABLES ----

    def show_tables_query(self, items: list[Any]) -> str:
        tables = self.db.get_tables()
        d = "------------------------"
        lines = [d] + tables + [d]
        n = len(tables)
        lines.append(f"{n} row{'s' if n != 1 else ''} in set")
        return "\n".join(lines)

    # ---- DDL: RENAME TABLE ----

    def rename_table_query(self, items: list[Any]) -> str:
        names = _strs(items)
        old, new = names[0], names[1]
        if not self.db.has_table(old):
            return "Rename table has failed: no such table"
        if self.db.has_table(new):
            return (
                f"Rename table has failed: " f"there is already a table named '{new}'"
            )
        self.db.rename_table(old, new)
        return f"'{new}' is renamed"

    # ---- DDL: TRUNCATE TABLE ----

    def truncate_table_query(self, items: list[Any]) -> str:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return "Truncate table has failed: no such table"
        if self.db.is_referenced(t):
            return (
                f"Truncate table has failed: " f"'{t}' is referenced by another table"
            )
        self.db.clear_rows(t)
        return f"'{t}' is truncated"

    # ---- DML: INSERT ----

    def insert_query(self, items: list[Any]) -> str:
        parts = [i for i in items if not isinstance(i, Token)]
        tname: str = parts[0]

        if not self.db.has_table(tname):
            return "Insert has failed: no such table"

        schema = self.db.get_schema(tname)
        columns: list[dict[str, Any]] = schema["columns"]

        # Determine if column list is present.
        if len(parts) == 3:
            col_list: list[str] = parts[1]
            vals: list[Any] = parts[2]
        else:
            col_list = []
            vals = parts[1]

        # Build row in table column order.
        if not col_list:
            row: list[Any] = list(vals)
        else:
            row = [None] * len(columns)
            idx = {c["name"]: i for i, c in enumerate(columns)}
            for c, v in zip(col_list, vals):
                row[idx[c]] = v

        # Truncate char values exceeding max length.
        for i, c in enumerate(columns):
            if c["type"][0] == "char" and isinstance(row[i], str):
                ml: int = c["type"][1]
                if len(row[i]) > ml:
                    row[i] = row[i][:ml]

        self.db.add_row(tname, row)
        return "The row is inserted"

    # ---- DML: SELECT ----

    def select_query(self, items: list[Any]) -> str:
        parts = [i for i in items if not isinstance(i, Token)]
        tables: list[str] = parts[1]
        tname = tables[0]

        if not self.db.has_table(tname):
            return f"Select has failed: '{tname}' does not exist"

        schema = self.db.get_schema(tname)
        hdrs = [c["name"].upper() for c in schema["columns"]]
        rows = self.db.get_rows(tname)
        drows: list[list[str]] = [
            [str(v) if v is not None else "null" for v in r] for r in rows
        ]
        return _fmt_table(hdrs, drows)

    # ---- Unimplemented (pass-through for future projects) ----

    def delete_query(self, _: list[Any]) -> str:
        return "'DELETE' requested"

    def update_query(self, _: list[Any]) -> str:
        return "'UPDATE' requested"

    # ---- Control ----

    def exit_query(self, _: list[Any]) -> None:
        return None

    def query(self, args: list[Result]) -> Result:
        assert len(args) == 1
        return args[0]

    def command(self, args: list[Result]) -> Result:
        assert len(args) == 1
        return args[0]


def build_parser_concurrent():
    """
    Kick off parser construction in a background thread.
    """

    def task():
        try:
            filepath = os.path.join(os.path.dirname(__file__), GRAMMAR_FILE)
            with open(filepath, "r") as f:
                # Lexer MUST be basic to disallow keywords as identifiers.
                # No inline transformer; we transform after parsing.
                result = Lark(
                    f,
                    debug=DEBUG,
                    strict=DEBUG,
                    start="command",
                    lexer="basic",
                    parser="lalr",
                )
            parser_future.set_result(result)
        except Exception as e:
            parser_future.set_exception(e)

    thread = Thread(target=task, daemon=True)
    thread.start()


def ensure_parser():
    """
    Block until the parser is ready and set the global parser.
    """
    global parser
    if parser is None:
        parser = parser_future.result()


def process_seq(*seq: str) -> bool:
    """
    Takes a sequence of sql statements and prints each result. Returns `True` if
    the process should terminate. If syntax error is encountered in the middle
    of the statements, the remainings are ignored.
    """
    ensure_parser()
    assert parser is not None
    assert transformer is not None
    for s in seq:
        try:
            tree: Tree[Token] = parser.parse(s)
            res = cast(SQLTransformer.Result, transformer.transform(tree))
            if res is None:
                return True
            # Multi-line results get newline after prompt.
            if "\n" in res:
                sys.stdout.write(PROMPT + "\n" + res + "\n")
            else:
                sys.stdout.write(PROMPT + res + "\n")
        except UnexpectedInput:
            sys.stdout.write(PROMPT + "Syntax error\n")
            break  # Stop after failure.
    sys.stdout.flush()
    return False


def interact():
    """
    Process I/O.
    """
    buf: list[str] = []  # Buffer to memorize input state.
    while True:
        if sys.stdin.isatty() and len(buf) == 0:
            sys.stdout.write(PROMPT)
            sys.stdout.flush()

        line: str = sys.stdin.readline()
        if line == "":  # EOF
            break
        parts = [p.strip() for p in line.split(";")]
        assert len(parts) > 0

        # At least one semicolon (';') found, run the sql engine.
        if len(parts) > 1:
            # Build the first statement with the buffer.
            buf.append(parts[0])
            part0 = " ".join(buf)
            buf = []

            if process_seq(part0, *parts[1:-1]):
                break

        # Store the part after the last semicolon to the buffer.
        if len(parts[-1]) > 0:
            buf.append(parts[-1])
    return


if __name__ == "__main__":
    build_parser_concurrent()
    db = Database()
    transformer = SQLTransformer(db)
    try:
        interact()
    finally:
        db.close()
    sys.exit(0)
