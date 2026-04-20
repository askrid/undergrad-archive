# Project 1-2: Implementing DDL & Basic DML

## How-to

- **Python 3.12** should be installed in the system.
- You can run `make` to automatically setup the virtual environment and run the DBMS.
- You can run `make test` to automatically setup the virtual environment and run the tests.
- Or, you can manually setup the environment as `requirements.txt` is provided.

## Implementation

### `grammar.lark`

- Added `NULL` to the `value` rule to support explicit null values in INSERT statements.
- No other changes from Project 1-1.

### `run.py`

#### `Database` class

- Wraps LMDB with a simple key-value scheme for persistent storage.
  - `meta:tables` stores the ordered list of table names.
  - `schema:{name}` stores the full schema (columns, types, nullable, PK, FKs) as JSON.
  - `data:{name}` stores all rows as a JSON array.
- Provides methods for all CRUD operations: create, drop, rename, insert, truncate, etc.
- `is_referenced()` scans all tables' FK definitions to check if a table is referenced.
- `rename_table()` also updates FK references in other tables pointing to the old name.

#### `SQLTransformer` class

- Replaces the `NoopTransformer` from Project 1-1.
- Low-level methods (`table_name`, `column_name`, `data_type`, etc.) extract structured data from the parse tree bottom-up.
- Query-level methods (`create_table_query`, `drop_table_query`, etc.) validate and execute against the `Database`.
- CREATE TABLE performs all 9 error checks (CharLength, DuplicateColumn, DuplicatePK, PKColumnDef, FKColumnDef, TableExistence, ReferenceExistence, ReferenceType, ReferenceNonPK).
- INSERT truncates char values exceeding the declared length and supports optional column lists.
- The parser is no longer created with an inline transformer; it parses to a tree first, then `MyTransformer.transform()` executes the query.

#### `_fmt_table()`

- Formats tabular output (EXPLAIN, SELECT) with dynamic column widths, separators, and row count.

### `test.sh`

- Test cases covering all DDL/DML commands, error types, persistence, case insensitivity, and edge cases.

## My Impression

- Designing the schema storage format on a key-value store was an interesting exercise.
- `Token extends str` issue was a subtle bug to catch.

