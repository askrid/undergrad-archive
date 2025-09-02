def E(table: list[set[int]], qset: set[int]):
    """
    Implementation of E() from the lecture.
    """
    if len(qset) == 0:
        return set()

    new = qset.copy()
    while True:
        tmp: set[int] = set()
        for q in new:
            tmp.update(table[3 * q])
        new = tmp.difference(qset)
        qset.update(new)
        if len(new) == 0:
            break

    return qset


def run(table: list[set[int]], q_ini: int, q_fin: int, string: str):
    """
    Run the NFA and returns a booelan value representing whether the NFA accepts the input string.
    """
    char_table = {"0": 1, "1": 2}
    qset = E(table, {q_ini})  # A set containing the reachable states

    for c in string:
        tmp: set[int] = set()
        for q in qset:
            tmp.update(table[3 * q + char_table[c]])
        qset = E(table, tmp)

    return q_fin in qset


if __name__ == "__main__":
    m = int(input().strip())
    strings = input().strip().split(" ")
    n, q_ini, q_fin = map(lambda x: int(x), input().strip().split(" "))

    # Build a transition table.
    table: list[set[int]] = []
    for i in range(n * 3):
        table.append(set(map(lambda x: int(x), input().strip().split(" ")[1:])))

    # Run the NFA and print the result for each case.
    for s in strings:
        result = run(table, q_ini, q_fin, s)
        if result:
            print("yes")
        else:
            print("no")
