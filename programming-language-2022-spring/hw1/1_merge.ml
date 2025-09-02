let rec merge: int list * int list -> int list = fun (list1, list2) -> (
  match (list1, list2) with
  | ([], h::t) -> h::t
  | (h::t, []) -> h::t
  | ([], []) -> []
  | (head1::tail1, head2::tail2) -> (
    if (head1 >= head2) then (
      head1 :: merge (tail1, list2)
    ) else (
      head2 :: merge (list1, tail2)
    )
  )
)
