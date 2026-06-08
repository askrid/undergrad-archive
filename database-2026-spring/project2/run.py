import csv
import sys
from contextlib import contextmanager
from pathlib import Path
from typing import Any, cast

import numpy as np
from mysql.connector import connect

DB_CONFIG = dict(
    host="astronaut.snu.ac.kr",
    port=7003,
    user="DB2020_17316",
    password="DB2020_17316",
    database="DB2020_17316",
    charset="utf8mb4",
    collation="utf8mb4_bin",
)

DATA_CSV = Path(__file__).resolve().parent / "data.csv"
SEP = "-" * 80
MAX_LOANS = 3
OVERDUE_THRESHOLD = 5
PENALTY_ATTEMPTS = 2

Row = tuple[Any, ...]
_conn = None


# ---------- connection / cursor ----------


def connection():
    global _conn
    if _conn is None or not _conn.is_connected():
        _conn = connect(**DB_CONFIG)
    return _conn


@contextmanager
def cursor():
    cur = connection().cursor(buffered=True)
    try:
        yield cur
    finally:
        cur.close()


def fetch_one(cur) -> Row | None:
    row = cur.fetchone()
    return cast(tuple, row) if row is not None else None


def fetch_all(cur) -> list[Row]:
    return [cast(tuple, r) for r in cur.fetchall()]


def fetch_required(cur) -> Row:
    row = fetch_one(cur)
    assert row is not None
    return row


def commit():
    connection().commit()


# ---------- formatting / parsing ----------


def fmt_avg(v: Any) -> str:
    if v is None:
        return "None"
    r = round(float(v), 1)
    return str(int(r)) if r == int(r) else f"{r:g}"


def print_table(header: list[str], rows) -> None:
    print(SEP)
    print(" ".join(header))
    print(SEP)
    for r in rows:
        print(" ".join(str(c) for c in r))
    print(SEP)


def parse_int(s: str) -> int | None:
    try:
        return int(s)
    except ValueError:
        return None


# ---------- schema ----------

DDL = (
    """CREATE TABLE IF NOT EXISTS dvds (
        d_id INT AUTO_INCREMENT PRIMARY KEY,
        d_title VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
        d_name VARCHAR(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
        age_limit TINYINT NOT NULL,
        stock INT NOT NULL DEFAULT 2,
        cumul_rent_cnt INT NOT NULL DEFAULT 0,
        UNIQUE KEY uq_dvd (d_title, d_name)
    ) ENGINE=InnoDB""",
    """CREATE TABLE IF NOT EXISTS users (
        u_id INT AUTO_INCREMENT PRIMARY KEY,
        u_name VARCHAR(30) CHARACTER SET utf8mb4 NOT NULL,
        u_age INT NOT NULL,
        overdue INT NOT NULL DEFAULT 0,
        penalty_left INT NOT NULL DEFAULT 0,
        restricted TINYINT NOT NULL DEFAULT 0,
        cumul_rent_cnt INT NOT NULL DEFAULT 0
    ) ENGINE=InnoDB""",
    """CREATE TABLE IF NOT EXISTS ratings (
        u_id INT NOT NULL, d_id INT NOT NULL, rating TINYINT NOT NULL,
        PRIMARY KEY (u_id, d_id),
        FOREIGN KEY (u_id) REFERENCES users(u_id) ON DELETE CASCADE,
        FOREIGN KEY (d_id) REFERENCES dvds(d_id) ON DELETE CASCADE
    ) ENGINE=InnoDB""",
    """CREATE TABLE IF NOT EXISTS borrowings (
        u_id INT NOT NULL, d_id INT NOT NULL,
        PRIMARY KEY (u_id, d_id),
        FOREIGN KEY (u_id) REFERENCES users(u_id) ON DELETE CASCADE,
        FOREIGN KEY (d_id) REFERENCES dvds(d_id) ON DELETE CASCADE
    ) ENGINE=InnoDB""",
    """CREATE TABLE IF NOT EXISTS reservations (
        d_id INT NOT NULL PRIMARY KEY, u_id INT NOT NULL,
        FOREIGN KEY (u_id) REFERENCES users(u_id) ON DELETE CASCADE,
        FOREIGN KEY (d_id) REFERENCES dvds(d_id) ON DELETE CASCADE
    ) ENGINE=InnoDB""",
)
TABLES = ("reservations", "borrowings", "ratings", "users", "dvds")

AVG_SUBQ = "(SELECT AVG(r.rating) FROM ratings r WHERE r.d_id = d.d_id)"


def create_schema(cur) -> None:
    for sql in DDL:
        cur.execute(sql)


def any_known_table_exists(cur) -> bool:
    cur.execute(
        "SELECT 1 FROM information_schema.tables "
        "WHERE table_schema=%s AND table_name IN (%s,%s,%s,%s,%s) LIMIT 1",
        (DB_CONFIG["database"], *TABLES),
    )
    return fetch_one(cur) is not None


def drop_schema(cur) -> None:
    cur.execute("SET FOREIGN_KEY_CHECKS=0")
    for t in TABLES:
        cur.execute(f"DROP TABLE IF EXISTS {t}")
    cur.execute("SET FOREIGN_KEY_CHECKS=1")


def load_csv(cur) -> None:
    dvds: dict[int, tuple[str, str, int]] = {}
    users: dict[int, tuple[str, int]] = {}
    ratings: list[tuple[int, int, int]] = []
    with DATA_CSV.open(newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            d, u = int(row["d_id"]), int(row["u_id"])
            dvds.setdefault(d, (row["d_title"], row["d_name"], int(row["age_limit"])))
            users.setdefault(u, (row["u_name"], int(row["u_age"])))
            ratings.append((u, d, int(row["rating"])))

    d_counts = dict.fromkeys(dvds, 0)
    u_counts = dict.fromkeys(users, 0)
    for u, d, _ in ratings:
        d_counts[d] += 1
        u_counts[u] += 1

    cur.executemany(
        "INSERT INTO dvds (d_id, d_title, d_name, age_limit, cumul_rent_cnt) "
        "VALUES (%s,%s,%s,%s,%s)",
        [(i, t, n, a, d_counts[i]) for i, (t, n, a) in sorted(dvds.items())],
    )
    cur.executemany(
        "INSERT INTO users (u_id, u_name, u_age, cumul_rent_cnt) VALUES (%s,%s,%s,%s)",
        [(i, n, a, u_counts[i]) for i, (n, a) in sorted(users.items())],
    )
    cur.executemany(
        "INSERT IGNORE INTO ratings (u_id, d_id, rating) VALUES (%s,%s,%s)", ratings
    )
    if dvds:
        cur.execute(f"ALTER TABLE dvds AUTO_INCREMENT = {max(dvds) + 1}")
    if users:
        cur.execute(f"ALTER TABLE users AUTO_INCREMENT = {max(users) + 1}")


# ---------- domain helpers ----------


def user_loads(cur, u_id: int) -> tuple[int, int]:
    cur.execute(
        "SELECT (SELECT COUNT(*) FROM borrowings WHERE u_id=%s), "
        "(SELECT COUNT(*) FROM reservations WHERE u_id=%s)",
        (u_id, u_id),
    )
    b, r = fetch_required(cur)
    return int(b), int(r)


def require_dvd(cur, d_id: int) -> bool:
    cur.execute("SELECT 1 FROM dvds WHERE d_id=%s", (d_id,))
    if fetch_one(cur) is None:
        print(f"DVD {d_id} does not exist")
        return False
    return True


def require_user(cur, u_id: int) -> bool:
    cur.execute("SELECT 1 FROM users WHERE u_id=%s", (u_id,))
    if fetch_one(cur) is None:
        print(f"User {u_id} does not exist")
        return False
    return True


def read_id(prompt: str, kind: str) -> int | None:
    raw = input(prompt).strip()
    val = parse_int(raw)
    if val is None:
        print(f"{kind} {raw} does not exist")
        return None
    return val


def do_borrow(cur, d_id: int, u_id: int) -> None:
    cur.execute("INSERT INTO borrowings (u_id, d_id) VALUES (%s,%s)", (u_id, d_id))
    cur.execute(
        "UPDATE dvds SET stock = stock - 1, cumul_rent_cnt = cumul_rent_cnt + 1 "
        "WHERE d_id=%s",
        (d_id,),
    )
    cur.execute(
        "UPDATE users SET cumul_rent_cnt = cumul_rent_cnt + 1 WHERE u_id=%s", (u_id,)
    )


# ---------- menu: schema management ----------


def initialize_database():
    with cursor() as cur:
        if any_known_table_exists(cur):
            print("Database already initialized. Use menu 17 to reset.")
            return
        create_schema(cur)
        load_csv(cur)
        commit()
    print("Database successfully initialized")


def reset():
    ans = input("All tables and data will be reset. Continue? (y/n): ").strip().lower()
    if ans != "y":
        return
    with cursor() as cur:
        drop_schema(cur)
        create_schema(cur)
        load_csv(cur)
        commit()
    print("Database successfully initialized")


# ---------- menu: printing ----------


def print_DVDs():
    with cursor() as cur:
        cur.execute(f"""SELECT d.d_id, d.d_title, d.d_name, d.age_limit, {AVG_SUBQ},
                       d.cumul_rent_cnt, d.stock,
                       (SELECT COUNT(*) FROM reservations rv WHERE rv.d_id = d.d_id)
                FROM dvds d ORDER BY d.d_id""")
        rows = [
            (i, t, n, a, fmt_avg(v), c, s, w)
            for i, t, n, a, v, c, s, w in fetch_all(cur)
        ]
    print_table(
        [
            "id",
            "title",
            "director",
            "age_limit",
            "avg.rating",
            "cumul_rent_cnt",
            "stock",
            "waiting",
        ],
        rows,
    )


def print_users():
    with cursor() as cur:
        cur.execute("""SELECT u.u_id, u.u_name, u.u_age,
                      (SELECT AVG(r.rating) FROM ratings r WHERE r.u_id = u.u_id),
                      u.cumul_rent_cnt
               FROM users u ORDER BY u.u_id""")
        rows = [(i, n, a, fmt_avg(v), c) for i, n, a, v, c in fetch_all(cur)]
    print_table(["id", "name", "age", "avg.rating", "cumul_rent_cnt"], rows)


def print_borrowing_status_for_user():
    u_id = read_id("User ID: ", "User")
    if u_id is None:
        return
    with cursor() as cur:
        if not require_user(cur, u_id):
            return
        cur.execute(
            f"""SELECT d.d_id, d.d_title, d.d_name, d.age_limit, {AVG_SUBQ}
                FROM borrowings b JOIN dvds d ON b.d_id = d.d_id
                WHERE b.u_id=%s ORDER BY d.d_id""",
            (u_id,),
        )
        rows = [(i, t, n, a, fmt_avg(v)) for i, t, n, a, v in fetch_all(cur)]
    print_table(["id", "title", "director", "age_limit", "avg.rating"], rows)


# ---------- menu: CRUD ----------


def insert_DVD():
    title = input("DVD title: ")
    director = input("director: ")
    age_in = input("Age limit: ")
    if not (1 <= len(title) <= 50):
        print("Title length should range from 1 to 50 characters")
        return
    if not (1 <= len(director) <= 30):
        print("Director length should range from 1 to 30 characters")
        return
    age = parse_int(age_in)
    if age is None or not (0 <= age <= 19):
        print("Age limit should be an integer from 0 to 19")
        return
    with cursor() as cur:
        cur.execute(
            "SELECT 1 FROM dvds WHERE d_title=%s AND d_name=%s", (title, director)
        )
        if fetch_one(cur):
            print(f"DVD ({title}, {director}) already exists")
            return
        cur.execute(
            "INSERT INTO dvds (d_title, d_name, age_limit) VALUES (%s,%s,%s)",
            (title, director, age),
        )
        commit()
    print("DVD successfully inserted")


def remove_DVD():
    d_id = read_id("DVD ID: ", "DVD")
    if d_id is None:
        return
    with cursor() as cur:
        if not require_dvd(cur, d_id):
            return
        cur.execute("SELECT 1 FROM borrowings WHERE d_id=%s LIMIT 1", (d_id,))
        if fetch_one(cur):
            print("Cannot delete a DVD that is currently borrowed")
            return
        cur.execute("DELETE FROM dvds WHERE d_id=%s", (d_id,))
        commit()
    print("DVD successfully removed")


def insert_user():
    name = input("User name: ")
    age_in = input("User age: ")
    if not (1 <= len(name) <= 30):
        print("Username length should range from 1 to 30 characters")
        return
    age = parse_int(age_in)
    if age is None or age <= 0:
        print("Age should be a positive integer")
        return
    with cursor() as cur:
        cur.execute("INSERT INTO users (u_name, u_age) VALUES (%s,%s)", (name, age))
        commit()
    print("One user successfully inserted")


def remove_user():
    u_id = read_id("User ID: ", "User")
    if u_id is None:
        return
    with cursor() as cur:
        if not require_user(cur, u_id):
            return
        cur.execute("SELECT 1 FROM borrowings WHERE u_id=%s LIMIT 1", (u_id,))
        if fetch_one(cur):
            print("Cannot delete a user with borrowed DVDs")
            return
        cur.execute("DELETE FROM users WHERE u_id=%s", (u_id,))
        commit()
    print("One user successfully removed")


# ---------- menu: checkout / return / reservation ----------


def reservation_flow(cur, d_id: int, age_limit: int) -> None:
    u_id = read_id("User ID: ", "User")
    if u_id is None:
        return
    cur.execute("SELECT u_age, restricted FROM users WHERE u_id=%s", (u_id,))
    row = fetch_one(cur)
    if not row:
        print(f"User {u_id} does not exist")
        return
    u_age, restricted = row
    if u_age < age_limit:
        print(f"User {u_id} does not meet the age limit for this DVD")
        return
    if restricted:
        print("Cannot make a reservation while under penalty.")
        return
    b, r = user_loads(cur, u_id)
    if b + r >= MAX_LOANS:
        print(
            "Cannot make a reservation. "
            "The combined limit of rentals and reservations is 3."
        )
        return
    cur.execute("SELECT 1 FROM reservations WHERE d_id=%s", (d_id,))
    if fetch_one(cur):
        print("This DVD already has a reservation")
        return
    cur.execute("INSERT INTO reservations (d_id, u_id) VALUES (%s,%s)", (d_id, u_id))
    commit()
    print("DVD reservation successfully registered")


def bump_overdue_for_dvd(cur, d_id: int) -> None:
    cur.execute(
        "SELECT u_id, overdue, restricted FROM users "
        "WHERE u_id IN (SELECT u_id FROM borrowings WHERE d_id=%s)",
        (d_id,),
    )
    for bu, overdue, restricted in fetch_all(cur):
        new_overdue = overdue + 1
        if not restricted and new_overdue >= OVERDUE_THRESHOLD:
            cur.execute(
                "UPDATE users SET overdue=%s, restricted=1, penalty_left=%s "
                "WHERE u_id=%s",
                (new_overdue, PENALTY_ATTEMPTS, bu),
            )
        else:
            cur.execute("UPDATE users SET overdue=%s WHERE u_id=%s", (new_overdue, bu))


def checkout_DVD():
    d_id = read_id("DVD ID: ", "DVD")
    if d_id is None:
        return
    with cursor() as cur:
        cur.execute("SELECT stock, age_limit FROM dvds WHERE d_id=%s", (d_id,))
        dvd = fetch_one(cur)
        if not dvd:
            print(f"DVD {d_id} does not exist")
            return
        stock, age_limit = dvd

        if stock == 0:
            if input("Reserve? (y/n): ").strip().lower() != "y":
                return
            reservation_flow(cur, d_id, age_limit)
            return

        u_id = read_id("User ID: ", "User")
        if u_id is None:
            return
        cur.execute(
            "SELECT u_age, restricted, penalty_left FROM users WHERE u_id=%s",
            (u_id,),
        )
        urow = fetch_one(cur)
        if not urow:
            print(f"User {u_id} does not exist")
            return
        u_age, restricted, penalty_left = urow

        b, r = user_loads(cur, u_id)
        if b + r >= MAX_LOANS:
            print(f"User {u_id} exceeded the maximum borrowing limit")
            return
        if u_age < age_limit:
            print(f"User {u_id} does not meet the age limit for this DVD")
            return
        if restricted:
            new_left = penalty_left - 1
            if new_left <= 0:
                cur.execute(
                    "UPDATE users SET restricted=0, penalty_left=0, overdue=0 "
                    "WHERE u_id=%s",
                    (u_id,),
                )
            else:
                cur.execute(
                    "UPDATE users SET penalty_left=%s WHERE u_id=%s",
                    (new_left, u_id),
                )
            commit()
            print(
                f"User {u_id} is currently restricted from borrowing DVDs "
                f"({penalty_left} attempts left)"
            )
            return

        bump_overdue_for_dvd(cur, d_id)
        do_borrow(cur, d_id, u_id)
        commit()
    print("DVD successfully checked out")


def auto_checkout_reserver(cur, d_id: int) -> None:
    """Promote pending reservation to a borrow if eligible. Caller must have
    already incremented stock by 1; success path does the matching decrement."""
    cur.execute("SELECT u_id FROM reservations WHERE d_id=%s", (d_id,))
    rv = fetch_one(cur)
    if not rv:
        return
    (res_u,) = rv
    cur.execute("DELETE FROM reservations WHERE d_id=%s", (d_id,))
    cur.execute(
        "SELECT restricted, (SELECT COUNT(*) FROM borrowings WHERE u_id=%s) "
        "FROM users WHERE u_id=%s",
        (res_u, res_u),
    )
    restricted, borrows = fetch_required(cur)
    if not restricted and borrows < MAX_LOANS:
        do_borrow(cur, d_id, res_u)
        print(f"DVD {d_id} has been automatically checked out for User {res_u}")


def return_and_rate_DVD():
    raw_d = input("DVD ID: ").strip()
    raw_u = input("User ID: ").strip()
    raw_r = input("Rating (1~5): ").strip()
    d_id = parse_int(raw_d)
    if d_id is None:
        print(f"DVD {raw_d} does not exist")
        return
    u_id = parse_int(raw_u)
    if u_id is None:
        print(f"User {raw_u} does not exist")
        return
    with cursor() as cur:
        if not require_dvd(cur, d_id):
            return
        if not require_user(cur, u_id):
            return
        rating = parse_int(raw_r)
        if rating is None or not (1 <= rating <= 5):
            print("Rating should be an integer from 1 to 5")
            return
        cur.execute("SELECT 1 FROM borrowings WHERE u_id=%s AND d_id=%s", (u_id, d_id))
        if not fetch_one(cur):
            print(
                "Cannot return and rate a DVD that is "
                "not currently borrowed for this user"
            )
            return

        cur.execute("DELETE FROM borrowings WHERE u_id=%s AND d_id=%s", (u_id, d_id))
        cur.execute(
            "INSERT INTO ratings (u_id, d_id, rating) VALUES (%s,%s,%s) "
            "ON DUPLICATE KEY UPDATE rating=VALUES(rating)",
            (u_id, d_id, rating),
        )
        print("DVD successfully returned and rated")
        cur.execute("UPDATE dvds SET stock = stock + 1 WHERE d_id=%s", (d_id,))
        auto_checkout_reserver(cur, d_id)
        commit()


def cancel_reservation():
    d_id = read_id("DVD ID: ", "DVD")
    if d_id is None:
        return
    with cursor() as cur:
        if not require_dvd(cur, d_id):
            return
        cur.execute("SELECT 1 FROM reservations WHERE d_id=%s", (d_id,))
        if not fetch_one(cur):
            print(f"Reservation for DVD {d_id} does not exist.")
            return
        cur.execute("DELETE FROM reservations WHERE d_id=%s", (d_id,))
        commit()
    print(f"Reservation for DVD {d_id} successfully cancelled")


# ---------- menu: search / recommendations ----------


def search():
    query = input("Query: ")
    with cursor() as cur:
        cur.execute(
            f"""SELECT d.d_id, d.d_title, d.d_name, d.age_limit, {AVG_SUBQ}, d.stock,
                       (SELECT COUNT(*) FROM reservations rv WHERE rv.d_id = d.d_id)
                FROM dvds d WHERE LOWER(d.d_title) LIKE LOWER(%s)
                ORDER BY d.d_id""",
            (f"%{query}%",),
        )
        rows = [
            (i, t, n, a, fmt_avg(v), s, w) for i, t, n, a, v, s, w in fetch_all(cur)
        ]
    if not rows:
        print("Cannot find any matching results")
        return
    print_table(
        ["id", "title", "director", "age_limit", "avg.rating", "stock", "waiting"],
        rows,
    )


def recommend_popularity():
    u_id = read_id("User ID: ", "User")
    if u_id is None:
        return
    with cursor() as cur:
        cur.execute("SELECT u_age FROM users WHERE u_id=%s", (u_id,))
        urow = fetch_one(cur)
        if not urow:
            print(f"User {u_id} does not exist")
            return
        u_age = urow[0]
        cur.execute(
            f"""SELECT d.d_id, d.d_title, d.d_name, d.age_limit, {AVG_SUBQ},
                       d.cumul_rent_cnt, d.stock
                FROM dvds d
                WHERE d.age_limit <= %s
                  AND d.d_id NOT IN (SELECT d_id FROM ratings WHERE u_id=%s)
                ORDER BY d.d_id""",
            (u_age, u_id),
        )
        candidates = fetch_all(cur)
    if not candidates:
        print("No DVD can be recommended")
        return

    rated = [c for c in candidates if c[4] is not None]
    rating_pick = (
        max(rated, key=lambda c: (float(c[4]), -c[0])) if rated else candidates[0]
    )
    pop_pick = max(candidates, key=lambda c: (c[5], -c[0]))

    print("Rating-based")
    print_table(
        ["id", "title", "director", "age_limit", "avg.rating", "stock"],
        [
            (
                rating_pick[0],
                rating_pick[1],
                rating_pick[2],
                rating_pick[3],
                fmt_avg(rating_pick[4]),
                rating_pick[6],
            )
        ],
    )
    print("Popularity-based")
    print_table(
        ["id", "title", "director", "age_limit", "cumul_rent_cnt", "stock"],
        [
            (
                pop_pick[0],
                pop_pick[1],
                pop_pick[2],
                pop_pick[3],
                pop_pick[5],
                pop_pick[6],
            )
        ],
    )


def recommend_user_based():
    u_id = read_id("User ID: ", "User")
    if u_id is None:
        return
    with cursor() as cur:
        cur.execute("SELECT u_age FROM users WHERE u_id=%s", (u_id,))
        urow = fetch_one(cur)
        if not urow:
            print(f"User {u_id} does not exist")
            return
        u_age = int(urow[0])
        cur.execute("SELECT u_id FROM users ORDER BY u_id")
        user_ids = [int(r[0]) for r in fetch_all(cur)]
        cur.execute("SELECT d_id, age_limit FROM dvds ORDER BY d_id")
        dvds = [(int(d), int(a)) for d, a in fetch_all(cur)]
        cur.execute("SELECT u_id, d_id, rating FROM ratings")
        rating_rows = [(int(u), int(d), float(r)) for u, d, r in fetch_all(cur)]

    if not user_ids or not dvds:
        print("No DVD can be recommended")
        return

    u_idx = {u: i for i, u in enumerate(user_ids)}
    d_idx = {d: j for j, (d, _) in enumerate(dvds)}
    n, m = len(user_ids), len(dvds)
    target_i = u_idx[u_id]

    rated_mask = np.zeros((n, m), dtype=bool)
    matrix = np.zeros((n, m), dtype=float)
    for u, d, rv in rating_rows:
        rated_mask[u_idx[u], d_idx[d]] = True
        matrix[u_idx[u], d_idx[d]] = rv

    candidates = [
        (d, d_idx[d])
        for d, a in dvds
        if a <= u_age and not rated_mask[target_i, d_idx[d]]
    ]
    if not candidates:
        print("No DVD can be recommended")
        return

    counts = rated_mask.sum(axis=1)
    sums = matrix.sum(axis=1)
    user_avgs = np.where(counts > 0, sums / np.maximum(counts, 1), 0.0)
    matrix = np.where(rated_mask, matrix, user_avgs[:, None])

    target_vec = matrix[target_i]
    norms = np.linalg.norm(matrix, axis=1)
    denom = norms * norms[target_i]
    with np.errstate(divide="ignore", invalid="ignore"):
        sims = np.where(denom > 0, matrix @ target_vec / denom, 0.0)

    others = np.ones(n, dtype=bool)
    others[target_i] = False
    sim_others = sims[others]
    matrix_others = matrix[others]
    sim_sum = float(sim_others.sum())

    cand_d = np.array([d for d, _ in candidates])
    cand_j = np.array([j for _, j in candidates])
    if sim_sum == 0:
        exp_rs = np.zeros(len(candidates))
    else:
        exp_rs = (sim_others @ matrix_others[:, cand_j]) / sim_sum
    best = int(np.lexsort((cand_d, -exp_rs))[0])
    best_d, best_exp = int(cand_d[best]), float(exp_rs[best])

    with cursor() as cur:
        cur.execute(
            f"SELECT d.d_id, d.d_title, d.d_name, d.age_limit, {AVG_SUBQ} "
            "FROM dvds d WHERE d.d_id=%s",
            (best_d,),
        )
        d_id_c, title, director, age_lim, avg = fetch_required(cur)
    print_table(
        ["id", "title", "director", "age_limit", "avg.rating", "exp.rating"],
        [(d_id_c, title, director, age_lim, fmt_avg(avg), fmt_avg(best_exp))],
    )


# ---------- menu ----------

MENU: tuple[tuple[str, Any], ...] = (
    ("1. initialize database", initialize_database),
    ("2. print all DVDs", print_DVDs),
    ("3. print all users", print_users),
    ("4. insert a new DVD", insert_DVD),
    ("5. remove a DVD", remove_DVD),
    ("6. insert a new user", insert_user),
    ("7. remove a user", remove_user),
    ("8. check out a DVD", checkout_DVD),
    ("9. return and rate a DVD", return_and_rate_DVD),
    ("10. cancel a reservation", cancel_reservation),
    ("11. print borrowing status of a user", print_borrowing_status_for_user),
    ("12. search DVDs", search),
    ("13. search directors", None),
    (
        "14. recommend a DVD for a user using popularity-based method",
        recommend_popularity,
    ),
    (
        "15. recommend a DVD for a user using user-based collaborative filtering",
        recommend_user_based,
    ),
    ("16. exit", None),
    ("17. reset database", reset),
)


def main():
    while True:
        print("=" * 60)
        for label, _ in MENU:
            print(label)
        print("=" * 60)
        choice = parse_int(input("Select your action: "))
        if choice is None or not 1 <= choice <= len(MENU):
            print("Invalid action")
            continue
        if choice == 16:
            print("Bye!")
            return
        action = MENU[choice - 1][1]
        if action is not None:
            action()


if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        print()
        sys.exit(0)
