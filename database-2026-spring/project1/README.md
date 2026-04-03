# Project 1-1: SQL Parser

## How-to

- **Python 3.12** should be installed in the system.
- You can run `make` to automatically setup the virtual environment and run the interactive parser.
- You can run `make test` to automatically setup the virtual environment and run the test on the parser.
- Or, you can manually setup the environment as `requirements.txt` is provided.

## Implementation

### `grammar.lark`

- For cleaner syntax error handling implementation (e.g., to prevent handling I/O in transformer or maintaining the global transformer state), I changed the grammar to handle a single SQL statement only. This removed the `query_list` rule in the original skeleton code.
- Also made minor changes to make it a bit more efficient for the LALR parser.

### `run.py`

- `interact()` handles the interactive I/O routine. It reads stdin line-by-line, split it by semicolon, run parser if semicolon is present, and store the remaining part (after the last semicolon) to the input state buffer so that future input lines are concatenated to. This handles multi-line queries, sequential queries, and even multi-line sequential queries.
- As the grammar changed to parse a single statement (instead of a sequence), the `process_seq()` function handles the sequential parsing, given a sequence of raw statements. It stops on the first syntax error or `EXIT` command encounter.
- `NoopTransformer` reduces a query subtree into a single descriptive text as shown in the specification, and then populate the result up to the root (`command`). The result is `None` if the query is `EXIT`.
- The parser is created concurrently (`build_parser_concurrent()`) on startup. It is a minor detail, but building parser takes about 0.2s on my machine. Now the program immediately shows the input prompt without blocking.
- Another minor detail is that it handles EOF properly and does not print input prompt when stdin is piped.

### `test.sh`

- This bash script covers various test cases.

## My Impression

- It was a fun exercise to recap the SQL syntax!
- Well-defined grammar seems to be crucial to SQL engine's performance. We need to study PL.

