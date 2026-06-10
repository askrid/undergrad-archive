from mysql.connector import connect

from run import DB_CONFIG

TABLES = ("ratings", "borrowings", "reservations", "users", "dvds")


def main():
    conn = connect(**DB_CONFIG)
    try:
        cur = conn.cursor()
        cur.execute("SET FOREIGN_KEY_CHECKS=0")
        for t in TABLES:
            cur.execute(f"TRUNCATE TABLE {t}")
            print(f"truncated {t}")
        cur.execute("SET FOREIGN_KEY_CHECKS=1")
        conn.commit()
        cur.close()
    finally:
        conn.close()


if __name__ == "__main__":
    main()
