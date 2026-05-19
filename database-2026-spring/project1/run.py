import json
import lmdb
import sys
import os
from concurrent.futures import Future
from dataclasses import dataclass
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

    def set_rows(self, name: str, rows: list[list[Any]]) -> None:
        self._put(f"data:{name}", rows)

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


@dataclass
class Result:
    """Output of a transformed query.

    `prompt` controls whether the shell PROMPT prefix is written before `text`.
    """

    text: str
    prompt: bool = True


def _fmt_table(
    headers: list[str] | None,
    rows: list[list[str]],
    min_width: int = 0,
) -> Result:
    """Format tabular data with dashes, headers, rows, and row count."""
    sample = headers or (rows[0] if rows else [""])
    ncols = len(sample)
    sources: list[list[str]] = ([headers] if headers else []) + rows or [sample]
    widths = [max(len(str(r[i])) for r in sources) for i in range(ncols)]

    def frow(r: list[str]) -> str:
        return " | ".join(str(r[i]).ljust(widths[i]) for i in range(ncols))

    tw = max(sum(widths) + 3 * (ncols - 1), min_width)
    dash = "-" * tw
    lines = [dash]
    if headers:
        lines.append(frow(headers))
    for r in rows:
        lines.append(frow(r))
    lines.append(dash)
    n = len(rows)
    lines.append(f"{n} row{'s' if n != 1 else ''} in set")
    return Result("\n".join(lines), prompt=False)


def _strs(items: list[Any]) -> list[str]:
    """Extract plain strings (transformed rule results) from items,
    filtering out Token objects (which are str subclass)."""
    return [i for i in items if isinstance(i, str) and not isinstance(i, Token)]


# Sentinel for SELECT *.
SELECT_STAR = "*"


class _QueryError(Exception):
    """Recoverable semantic error during query execution, translated to a
    message string at the top of the SELECT / DELETE handler."""

    def __init__(self, code: str, **info: Any):
        super().__init__(code)
        self.code = code
        self.info = info


def _col_type(t: list[Any]) -> str:
    return t[0]


def _can_compare(t1: str, t2: str, op: str) -> bool:
    if t1 == "null" or t2 == "null":
        return False
    if t1 != t2:
        return False
    if op in ("=", "!="):
        return True
    return t1 in ("int", "date")


def _resolve_column(
    ref: dict[str, Any],
    scope: list[dict[str, Any]],
    clause: str,
) -> dict[str, Any]:
    """Bind a column reference to a FROM entry; raise _QueryError otherwise.
    Returned shape: {"alias", "col", "type"}."""
    t = ref.get("t")
    c = ref["c"]
    if t is not None:
        entries = [e for e in scope if e["alias"] == t]
        if not entries:
            raise _QueryError("TABLE_NOT_SPECIFIED", clause=clause)
        for cs in entries[0]["schema"]["columns"]:
            if cs["name"] == c:
                return {"alias": t, "col": c, "type": _col_type(cs["type"])}
        raise _QueryError("COLUMN_NOT_EXIST", clause=clause)
    matches: list[tuple[dict[str, Any], dict[str, Any]]] = []
    for e in scope:
        for cs in e["schema"]["columns"]:
            if cs["name"] == c:
                matches.append((e, cs))
    if not matches:
        raise _QueryError("COLUMN_NOT_EXIST", clause=clause)
    if len(matches) > 1:
        raise _QueryError("AMBIGUOUS", clause=clause)
    e, cs = matches[0]
    return {"alias": e["alias"], "col": c, "type": _col_type(cs["type"])}


def _bind_operand(
    op: dict[str, Any],
    scope: list[dict[str, Any]],
    clause: str,
) -> dict[str, Any]:
    if op["k"] == "val":
        return op
    bound = _resolve_column({"t": op.get("t"), "c": op["c"]}, scope, clause)
    return {"k": "col", **bound}


def _bind_expr(
    ast: dict[str, Any] | None,
    scope: list[dict[str, Any]],
    clause: str,
) -> dict[str, Any] | None:
    """Resolve column refs and validate comparison compatibility."""
    if ast is None:
        return None
    k = ast["k"]
    if k in ("and", "or"):
        return {
            "k": k,
            "a": _bind_expr(ast["a"], scope, clause),
            "b": _bind_expr(ast["b"], scope, clause),
        }
    if k == "not":
        return {"k": "not", "x": _bind_expr(ast["x"], scope, clause)}
    if k == "is_null":
        return {
            "k": "is_null",
            "col": _resolve_column(ast["col"], scope, clause),
            "negated": ast["negated"],
        }
    if k == "cmp":
        lhs = _bind_operand(ast["lhs"], scope, clause)
        rhs = _bind_operand(ast["rhs"], scope, clause)
        lt = lhs["type"] if lhs["k"] == "col" else lhs["t"]
        rt = rhs["type"] if rhs["k"] == "col" else rhs["t"]
        if not _can_compare(lt, rt, ast["op"]):
            raise _QueryError("INCOMPARABLE")
        return {"k": "cmp", "lhs": lhs, "rhs": rhs, "op": ast["op"]}
    raise ValueError(f"Unknown AST node: {ast}")


def _operand_val(op: dict[str, Any], env: dict[tuple[str, str], Any]) -> Any:
    if op["k"] == "val":
        return op["v"]
    return env[(op["alias"], op["col"])]


def _eval_expr(
    ast: dict[str, Any] | None,
    env: dict[tuple[str, str], Any],
) -> bool | None:
    """Three-valued logic. None for WHERE-absent matches everything."""
    if ast is None:
        return True
    k = ast["k"]
    if k == "and":
        a = _eval_expr(ast["a"], env)
        b = _eval_expr(ast["b"], env)
        if a is False or b is False:
            return False
        if a is None or b is None:
            return None
        return True
    if k == "or":
        a = _eval_expr(ast["a"], env)
        b = _eval_expr(ast["b"], env)
        if a is True or b is True:
            return True
        if a is None or b is None:
            return None
        return False
    if k == "not":
        x = _eval_expr(ast["x"], env)
        return None if x is None else not x
    if k == "is_null":
        v = env[(ast["col"]["alias"], ast["col"]["col"])]
        is_null = v is None
        return (not is_null) if ast["negated"] else is_null
    if k == "cmp":
        lv = _operand_val(ast["lhs"], env)
        rv = _operand_val(ast["rhs"], env)
        if lv is None or rv is None:
            return None
        op = ast["op"]
        return {
            "=": lv == rv,
            "!=": lv != rv,
            "<": lv < rv,
            "<=": lv <= rv,
            ">": lv > rv,
            ">=": lv >= rv,
        }[op]
    raise ValueError(f"Unknown AST node: {ast}")


def _scope_entry(name: str, alias: str, schema: dict[str, Any]) -> dict[str, Any]:
    return {"name": name, "alias": alias, "schema": schema}


def _row_env(entry: dict[str, Any], row: list[Any]) -> dict[tuple[str, str], Any]:
    alias = entry["alias"]
    return {
        (alias, c["name"]): row[i] for i, c in enumerate(entry["schema"]["columns"])
    }


def _aggregate(fn: str, vals: list[Any]) -> Any:
    """MAX/MIN return None on empty or all-null input.
    SUM returns 0 on empty / all-null / non-int values."""
    non_null = [v for v in vals if v is not None]
    if fn == "max":
        return max(non_null) if non_null else None
    if fn == "min":
        return min(non_null) if non_null else None
    if fn == "sum":
        ints = [v for v in non_null if isinstance(v, int) and not isinstance(v, bool)]
        return sum(ints) if ints else 0
    raise ValueError(f"Unknown aggregate: {fn}")


def _msg_for(e: _QueryError) -> Result:
    clause = e.info.get("clause", "")
    if e.code == "TABLE_NOT_SPECIFIED":
        return Result(
            f"{clause} clause trying to reference tables which are not specified"
        )
    if e.code == "COLUMN_NOT_EXIST":
        return Result(f"{clause} clause trying to reference non existing column")
    if e.code == "AMBIGUOUS":
        return Result(f"{clause} clause contains ambiguous column reference")
    if e.code == "INCOMPARABLE":
        return Result("Trying to compare incomparable columns or values")
    if e.code == "INVALID_LIMIT_OFFSET":
        return Result(
            "Select has failed: LIMIT/OFFSET clause should be a non-negative integer"
        )
    if e.code == "SELECT_COL_RESOLVE":
        return Result(f"Select has failed: fail to resolve '{e.info.get('col', '')}'")
    if e.code == "SELECT_TABLE_NOT_EXIST":
        return Result(f"Select has failed: '{e.info.get('table', '')}' does not exist")
    raise ValueError(f"Unhandled query error code: {e.code}")


class SQLTransformer(Transformer):  # type: ignore[type-arg]
    """
    Transforms parsed SQL tree and executes queries against the LMDB-backed
    database. The result is either a Result or None (EXIT).
    """

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

    def value(self, items: list[Token]) -> dict[str, Any]:
        t = items[0]
        if t.type == "INT":
            return {"v": int(str(t)), "t": "int"}
        if t.type == "STR":
            return {"v": str(t)[1:-1], "t": "char"}
        if t.type == "DATE":
            return {"v": str(t), "t": "date"}
        if t.type == "NULL":
            return {"v": None, "t": "null"}
        raise ValueError(f"Unknown value type: {t}")

    def value_list(self, items: list[Any]) -> list[dict[str, Any]]:
        return [i for i in items if isinstance(i, dict)]

    # ---- SELECT structural pieces ----

    def column_ref(self, items: list[Any]) -> dict[str, Any]:
        names = _strs(items)
        if len(names) == 2:
            return {"k": "ref", "t": names[0], "c": names[1]}
        return {"k": "ref", "t": None, "c": names[0]}

    def aggregate_func(self, items: list[Token]) -> str:
        return str(items[0]).lower()

    def selected_column(self, items: list[Any]) -> dict[str, Any]:
        agg: str | None = None
        alias: str | None = None
        col: dict[str, Any] | None = None
        for x in items:
            if isinstance(x, dict) and x.get("k") == "ref":
                col = x
            elif isinstance(x, str) and not isinstance(x, Token):
                if x in ("max", "min", "sum"):
                    agg = x
                else:
                    alias = x
        assert col is not None
        return {"agg": agg, "col": col, "alias": alias}

    def select_list(self, items: list[Any]) -> Any:
        cols = [i for i in items if isinstance(i, dict) and "col" in i]
        if not cols:
            return SELECT_STAR
        return cols

    def referred_table(self, items: list[Any]) -> dict[str, Any]:
        names = _strs(items)
        name = names[0]
        alias = names[1] if len(names) > 1 else name
        return {"k": "base", "name": name, "alias": alias}

    def join_condition(self, items: list[Any]) -> tuple[dict[str, Any], dict[str, Any]]:
        refs = [i for i in items if isinstance(i, dict) and i.get("k") == "ref"]
        return (refs[0], refs[1])

    def joined_table(self, items: list[Any]) -> dict[str, Any]:
        target = next(i for i in items if isinstance(i, dict) and i.get("k") == "base")
        cond = next(i for i in items if isinstance(i, tuple))
        return {
            "k": "join",
            "name": target["name"],
            "alias": target["alias"],
            "on": cond,
        }

    def table_reference_list(self, items: list[Any]) -> list[dict[str, Any]]:
        return [
            i for i in items if isinstance(i, dict) and i.get("k") in ("base", "join")
        ]

    def from_clause(self, items: list[Any]) -> list[dict[str, Any]]:
        return next(i for i in items if isinstance(i, list))

    # ---- WHERE AST builders ----

    def comparable_value(self, items: list[Token]) -> dict[str, Any]:
        t = items[0]
        if t.type == "INT":
            return {"k": "val", "v": int(str(t)), "t": "int"}
        if t.type == "STR":
            return {"k": "val", "v": str(t)[1:-1], "t": "char"}
        if t.type == "DATE":
            return {"k": "val", "v": str(t), "t": "date"}
        raise ValueError(f"Unknown literal: {t}")

    def comp_operand(self, items: list[Any]) -> dict[str, Any]:
        for i in items:
            if isinstance(i, dict) and i.get("k") in ("val", "ref"):
                return i
        raise ValueError("comp_operand: no operand found")

    def comp_op(self, items: list[Any]) -> str:
        # items contains the literal op as a Token (anonymous terminal).
        return str(items[0])

    def comparison_predicate(self, items: list[Any]) -> dict[str, Any]:
        operands = [
            i for i in items if isinstance(i, dict) and i.get("k") in ("val", "ref")
        ]
        op = next(i for i in items if isinstance(i, str) and not isinstance(i, Token))
        return {"k": "cmp", "lhs": operands[0], "rhs": operands[1], "op": op}

    def null_operation(self, items: list[Token]) -> bool:
        return any(isinstance(t, Token) and t.type == "NOT" for t in items)

    def null_predicate(self, items: list[Any]) -> dict[str, Any]:
        col = next(i for i in items if isinstance(i, dict) and i.get("k") == "ref")
        negated = next(i for i in items if isinstance(i, bool))
        return {
            "k": "is_null",
            "col": {"t": col["t"], "c": col["c"]},
            "negated": negated,
        }

    def predicate(self, items: list[Any]) -> dict[str, Any]:
        return next(i for i in items if isinstance(i, dict))

    def parenthesized_boolean_expr(self, items: list[Any]) -> dict[str, Any]:
        return next(i for i in items if isinstance(i, dict))

    def boolean_test(self, items: list[Any]) -> dict[str, Any]:
        return next(i for i in items if isinstance(i, dict))

    def boolean_factor(self, items: list[Any]) -> dict[str, Any]:
        inner = next(i for i in items if isinstance(i, dict))
        if any(isinstance(t, Token) and t.type == "NOT" for t in items):
            return {"k": "not", "x": inner}
        return inner

    def boolean_term(self, items: list[Any]) -> dict[str, Any]:
        parts = [i for i in items if isinstance(i, dict)]
        acc = parts[0]
        for p in parts[1:]:
            acc = {"k": "and", "a": acc, "b": p}
        return acc

    def boolean_expr(self, items: list[Any]) -> dict[str, Any]:
        parts = [i for i in items if isinstance(i, dict)]
        acc = parts[0]
        for p in parts[1:]:
            acc = {"k": "or", "a": acc, "b": p}
        return acc

    def where_clause(self, items: list[Any]) -> dict[str, Any]:
        return next(i for i in items if isinstance(i, dict))

    def order_by_clause(self, items: list[Any]) -> dict[str, Any]:
        col = next(i for i in items if isinstance(i, dict) and i.get("k") == "ref")
        direction = "asc"
        for t in items:
            if isinstance(t, Token) and t.type == "DESC":
                direction = "desc"
            elif isinstance(t, Token) and t.type == "ASC":
                direction = "asc"
        return {"col": col, "dir": direction}

    def group_by_clause(self, items: list[Any]) -> dict[str, Any]:
        col = next(i for i in items if isinstance(i, dict) and i.get("k") == "ref")
        return {"col": col}

    def limit_clause(self, items: list[Any]) -> tuple[str, int]:
        n = next(i for i in items if isinstance(i, Token) and i.type == "INT")
        return ("limit", int(str(n)))

    def offset_clause(self, items: list[Any]) -> tuple[str, int]:
        n = next(i for i in items if isinstance(i, Token) and i.type == "INT")
        return ("offset", int(str(n)))

    def table_expression(self, items: list[Any]) -> dict[str, Any]:
        out: dict[str, Any] = {
            "from": [],
            "where": None,
            "group": None,
            "order": None,
            "limit": None,
            "offset": None,
        }
        for i in items:
            if isinstance(i, list):
                out["from"] = i
            elif isinstance(i, tuple) and len(i) == 2 and i[0] in ("limit", "offset"):
                out[i[0]] = i[1]
            elif isinstance(i, dict):
                if "dir" in i:
                    out["order"] = i
                elif "k" in i:
                    out["where"] = i
                elif "col" in i:
                    out["group"] = i
        return out

    # ---- DDL: CREATE TABLE ----

    def create_table_query(self, items: list[Any]) -> Result:
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
                return Result("Char length should be over 0")

        # DuplicateColumnDefError
        if len(cnames) != len(set(cnames)):
            return Result("Create table has failed: column definition is duplicated")

        # DuplicatePrimaryKeyDefError
        if len(pks) > 1:
            return Result(
                "Create table has failed: primary key definition is duplicated"
            )

        pk_cols: list[str] = pks[0]["cols"] if pks else []

        # PrimaryKeyColumnDefError
        for pc in pk_cols:
            if pc not in cnames:
                return Result(
                    f"Create table has failed:"
                    f"cannot define non-existing column '{pc}' as primary key"
                )

        # ForeignKeyColumnDefError
        for fk in fks:
            for fc in fk["cols"]:
                if fc not in cnames:
                    return Result(
                        f"Create table has failed: "
                        f"cannot define non-existing column '{fc}' as foreign key"
                    )

        # TableExistenceError
        if self.db.has_table(tname):
            return Result(
                "Create table has failed: table with the same name already exists"
            )

        # ReferenceExistenceError
        for fk in fks:
            if not self.db.has_table(fk["ref"]):
                return Result(
                    "Create table has failed: "
                    "foreign key references non existing table or column"
                )
            rs = self.db.get_schema(fk["ref"])
            rc_names = [c["name"] for c in rs["columns"]]
            for rc in fk["rcols"]:
                if rc not in rc_names:
                    return Result(
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
                    return Result(
                        "Create table has failed: foreign key references wrong type"
                    )

        # ReferenceNonPrimaryKeyError
        for fk in fks:
            rs = self.db.get_schema(fk["ref"])
            rpk: list[str] = rs.get("primary_key", [])
            if sorted(fk["rcols"]) != sorted(rpk):
                return Result(
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
        return Result(f"'{tname}' table is created")

    # ---- DDL: DROP TABLE ----

    def drop_table_query(self, items: list[Any]) -> Result:
        tname: str = _strs(items)[0]
        if not self.db.has_table(tname):
            return Result("Drop table has failed: no such table")
        if self.db.is_referenced(tname):
            return Result(
                f"Drop table has failed: '{tname}' is referenced by another table"
            )
        self.db.drop_table(tname)
        return Result(f"'{tname}' table is dropped")

    # ---- DDL: EXPLAIN / DESCRIBE / DESC ----

    def _explain(self, tname: str) -> Result:
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
        # Metadata describe output is printed without the PROMPT prefix.
        return _fmt_table(hdrs, rows)

    def explain_query(self, items: list[Any]) -> Result:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return Result("Explain has failed: no such table")
        return self._explain(t)

    def describe_query(self, items: list[Any]) -> Result:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return Result("Describe has failed: no such table")
        return self._explain(t)

    def desc_query(self, items: list[Any]) -> Result:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return Result("Desc has failed: no such table")
        return self._explain(t)

    # ---- DDL: SHOW TABLES ----

    def show_tables_query(self, items: list[Any]) -> Result:
        rows = [[t] for t in self.db.get_tables()]
        return _fmt_table(None, rows, min_width=24)

    # ---- DDL: RENAME TABLE ----

    def rename_table_query(self, items: list[Any]) -> Result:
        names = _strs(items)
        old, new = names[0], names[1]
        if not self.db.has_table(old):
            return Result("Rename table has failed: no such table")
        if self.db.has_table(new):
            return Result(
                f"Rename table has failed: there is already a table named '{new}'"
            )
        self.db.rename_table(old, new)
        return Result(f"'{new}' is renamed")

    # ---- DDL: TRUNCATE TABLE ----

    def truncate_table_query(self, items: list[Any]) -> Result:
        t: str = _strs(items)[0]
        if not self.db.has_table(t):
            return Result("Truncate table has failed: no such table")
        if self.db.is_referenced(t):
            return Result(
                f"Truncate table has failed: '{t}' is referenced by another table"
            )
        self.db.clear_rows(t)
        return Result(f"'{t}' is truncated")

    # ---- DML: INSERT ----

    def insert_query(self, items: list[Any]) -> Result:
        tname: str = next(
            i for i in items if isinstance(i, str) and not isinstance(i, Token)
        )
        col_list: list[str] | None = next(
            (i for i in items if isinstance(i, list) and i and isinstance(i[0], str)),
            None,
        )
        vals: list[dict[str, Any]] = next(
            i for i in items if isinstance(i, list) and i and isinstance(i[0], dict)
        )

        if not self.db.has_table(tname):
            return Result("Insert has failed: no such table")

        columns: list[dict[str, Any]] = self.db.get_schema(tname)["columns"]
        col_index = {c["name"]: i for i, c in enumerate(columns)}

        if col_list is not None:
            for c in col_list:
                if c not in col_index:
                    return Result(f"Insert has failed: '{c}' does not exist")
            if len(col_list) != len(vals):
                return Result("Insert has failed: types are not matched")
        else:
            if len(vals) != len(columns):
                return Result("Insert has failed: types are not matched")

        row: list[Any] = [None] * len(columns)
        targets = col_list if col_list is not None else [c["name"] for c in columns]
        for cname, val in zip(targets, vals):
            i = col_index[cname]
            ct = _col_type(columns[i]["type"])
            if val["t"] != "null" and val["t"] != ct:
                return Result("Insert has failed: types are not matched")
            row[i] = val["v"]

        for i, c in enumerate(columns):
            if row[i] is None and c["not_null"]:
                return Result(f"Insert has failed: '{c['name']}' is not nullable")
            if c["type"][0] == "char" and isinstance(row[i], str):
                ml: int = c["type"][1]
                if len(row[i]) > ml:
                    row[i] = row[i][:ml]

        self.db.add_row(tname, row)
        return Result("1 row inserted")

    # ---- DML: SELECT ----

    def select_query(self, items: list[Any]) -> Result:
        from itertools import product

        sel: Any = next(
            (i for i in items if i == SELECT_STAR or isinstance(i, list)), None
        )
        texp: dict[str, Any] = next(
            i for i in items if isinstance(i, dict) and "from" in i
        )

        scope: list[dict[str, Any]] = []
        joins: list[tuple[dict[str, Any], dict[str, Any]]] = []
        for entry in texp["from"]:
            if not self.db.has_table(entry["name"]):
                return Result(f"Select has failed: '{entry['name']}' does not exist")
            scope.append(
                _scope_entry(
                    entry["name"], entry["alias"], self.db.get_schema(entry["name"])
                )
            )
            if entry["k"] == "join":
                joins.append(entry["on"])

        try:
            bound_joins: list[tuple[dict[str, Any], dict[str, Any]]] = []
            for lhs, rhs in joins:
                lb = _resolve_column({"t": lhs["t"], "c": lhs["c"]}, scope, "join")
                rb = _resolve_column({"t": rhs["t"], "c": rhs["c"]}, scope, "join")
                if lb["type"] != rb["type"]:
                    raise _QueryError("INCOMPARABLE")
                bound_joins.append((lb, rb))

            where_b = _bind_expr(texp["where"], scope, "where")

            order_b: dict[str, Any] | None = None
            if texp["order"] is not None:
                order_b = _resolve_column(
                    {"t": texp["order"]["col"]["t"], "c": texp["order"]["col"]["c"]},
                    scope,
                    "order by",
                )

            projection: list[dict[str, Any]] = []
            if sel == SELECT_STAR:
                multi = len(scope) > 1
                for e in scope:
                    for c in e["schema"]["columns"]:
                        header = f"{e['alias']}.{c['name']}" if multi else c["name"]
                        projection.append(
                            {
                                "header": header,
                                "alias": e["alias"],
                                "col": c["name"],
                                "agg": None,
                            }
                        )
            else:
                for sc in sel:
                    ref = sc["col"]
                    try:
                        b = _resolve_column(
                            {"t": ref["t"], "c": ref["c"]}, scope, "select"
                        )
                    except _QueryError:
                        raise _QueryError("SELECT_COL_RESOLVE", col=ref["c"])
                    base = f"{ref['t']}.{ref['c']}" if ref["t"] else ref["c"]
                    if sc["alias"]:
                        header = sc["alias"]
                    elif sc["agg"]:
                        header = f"{sc['agg']}({base})"
                    else:
                        header = base
                    projection.append(
                        {
                            "header": header,
                            "alias": b["alias"],
                            "col": b["col"],
                            "agg": sc["agg"],
                        }
                    )

            group_b: dict[str, Any] | None = None
            if texp["group"] is not None:
                group_b = _resolve_column(
                    {"t": texp["group"]["col"]["t"], "c": texp["group"]["col"]["c"]},
                    scope,
                    "group by",
                )
        except _QueryError as e:
            return _msg_for(e)

        has_agg = any(p["agg"] for p in projection)
        if (group_b is not None or has_agg) and sel != SELECT_STAR:
            for p in projection:
                if p["agg"]:
                    continue
                if group_b is None or (p["alias"], p["col"]) != (
                    group_b["alias"],
                    group_b["col"],
                ):
                    return Result(
                        f"Select has failed: column '{p['col']}' must either be "
                        f"included in the GROUP BY clause or be used in an aggregate function"
                    )

        limit = texp["limit"]
        offset = texp["offset"]
        if (limit is not None and limit < 0) or (offset is not None and offset < 0):
            return _msg_for(_QueryError("INVALID_LIMIT_OFFSET"))

        table_rows = [self.db.get_rows(e["name"]) for e in scope]
        envs: list[dict[tuple[str, str], Any]] = []
        for combo in product(*table_rows):
            env: dict[tuple[str, str], Any] = {}
            for entry, row in zip(scope, combo):
                env.update(_row_env(entry, row))
            if any(
                env[(lb["alias"], lb["col"])] is None
                or env[(rb["alias"], rb["col"])] is None
                or env[(lb["alias"], lb["col"])] != env[(rb["alias"], rb["col"])]
                for lb, rb in bound_joins
            ):
                continue
            if _eval_expr(where_b, env) is True:
                envs.append(env)

        if group_b is not None or has_agg:
            groups: dict[Any, list[dict[tuple[str, str], Any]]] = {}
            keys: list[Any] = []
            for env in envs:
                k = env[(group_b["alias"], group_b["col"])] if group_b else None
                if k not in groups:
                    groups[k] = []
                    keys.append(k)
                groups[k].append(env)

            out_rows: list[list[Any]] = []
            for k in keys:
                gevs = groups[k]
                row: list[Any] = []
                for p in projection:
                    if p["agg"]:
                        vals = [g[(p["alias"], p["col"])] for g in gevs]
                        row.append(_aggregate(p["agg"], vals))
                    else:
                        row.append(gevs[0][(p["alias"], p["col"])])
                out_rows.append(row)

            if order_b is not None:
                idx = next(
                    (
                        i
                        for i, p in enumerate(projection)
                        if not p["agg"]
                        and (p["alias"], p["col"]) == (order_b["alias"], order_b["col"])
                    ),
                    None,
                )
                if idx is not None:
                    rev = texp["order"]["dir"] == "desc"
                    nulls = [r for r in out_rows if r[idx] is None]
                    non_null = [r for r in out_rows if r[idx] is not None]
                    non_null.sort(key=lambda r: r[idx], reverse=rev)
                    out_rows = (nulls + non_null) if rev else (non_null + nulls)
        else:
            if order_b is not None:
                key = (order_b["alias"], order_b["col"])
                rev = texp["order"]["dir"] == "desc"
                nulls = [e for e in envs if e[key] is None]
                non_nulls = [e for e in envs if e[key] is not None]
                non_nulls.sort(key=lambda e: e[key], reverse=rev)
                envs = (nulls + non_nulls) if rev else (non_nulls + nulls)
            out_rows = [
                [env[(p["alias"], p["col"])] for p in projection] for env in envs
            ]

        if offset:
            out_rows = out_rows[offset:]
        if limit is not None:
            out_rows = out_rows[:limit]

        headers = [p["header"] for p in projection]
        drows = [["null" if v is None else str(v) for v in r] for r in out_rows]
        return _fmt_table(headers, drows)

    # ---- DML: DELETE ----

    def delete_query(self, items: list[Any]) -> Result:
        tname: str = next(
            i for i in items if isinstance(i, str) and not isinstance(i, Token)
        )
        if not self.db.has_table(tname):
            return Result("Delete has failed: no such table")

        where = next((i for i in items if isinstance(i, dict) and i.get("k")), None)
        scope = [_scope_entry(tname, tname, self.db.get_schema(tname))]
        try:
            where_b = _bind_expr(where, scope, "where")
        except _QueryError as e:
            return _msg_for(e)

        rows = self.db.get_rows(tname)
        matched = [
            r for r in rows if _eval_expr(where_b, _row_env(scope[0], r)) is True
        ]
        blocked = [r for r in matched if self._row_referenced(tname, r)]

        if blocked:
            return Result(
                f"'{len(matched)}' row(s) are not deleted due to referential integrity"
            )
        deleted_ids = {id(r) for r in matched}
        survivors = [r for r in rows if id(r) not in deleted_ids]
        self.db.set_rows(tname, survivors)
        return Result(f"'{len(matched)}' row(s) deleted")

    def _row_referenced(self, parent: str, parent_row: list[Any]) -> bool:
        ps = self.db.get_schema(parent)
        p_idx = {c["name"]: i for i, c in enumerate(ps["columns"])}
        for child in self.db.get_tables():
            if child == parent:
                continue
            cs = self.db.get_schema(child)
            c_idx = {c["name"]: i for i, c in enumerate(cs["columns"])}
            for fk in cs.get("foreign_keys", []):
                if fk["ref_table"] != parent:
                    continue
                fk_pos = [c_idx[c] for c in fk["columns"]]
                ref_pos = [p_idx[c] for c in fk["ref_columns"]]
                parent_vals = [parent_row[i] for i in ref_pos]
                if any(v is None for v in parent_vals):
                    continue
                for child_row in self.db.get_rows(child):
                    child_vals = [child_row[i] for i in fk_pos]
                    if child_vals == parent_vals:
                        return True
        return False

    # ---- DML: UPDATE (pass-through; not in spec for 1-3) ----

    def update_query(self, _: list[Any]) -> Result:
        return Result("'UPDATE' requested")

    # ---- Control ----

    def exit_query(self, _: list[Any]) -> None:
        return None

    def query(self, args: "list[Result | None]") -> "Result | None":
        assert len(args) == 1
        return args[0]

    def command(self, args: "list[Result | None]") -> "Result | None":
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
            res = cast("Result | None", transformer.transform(tree))
            if res is None:
                return True
            out = PROMPT if res.prompt else ""
            out += res.text + "\n"
            sys.stdout.write(out)
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
