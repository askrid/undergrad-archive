type heap = EMPTY | NODE of rank * value * heap * heap
and rank = int
and value = int

exception EmptyHeap

let rank h = match h with
| EMPTY -> -1
| NODE(r,_,_,_) -> r

let shake (x,lh,rh) = if (rank lh) >= (rank rh)
  then NODE(rank rh + 1, x, lh, rh)
  else NODE(rank lh + 1, x, rh, lh)

let rec merge: heap * heap -> heap = fun (h1, h2) -> (
  match (h1, h2) with
  | (EMPTY, EMPTY) -> EMPTY
  | (EMPTY, NODE(_,_,_,_)) -> h2
  | (NODE(_,_,_,_), EMPTY) -> h1
  | (NODE (r1, v1, lh1, rh1), NODE (r2, v2, lh2, rh2)) -> (
    if (v1 > v2) then (
      merge (h2, h1)
    ) else (
      shake (v1, lh1, merge (rh1, h2))
    )
  )
)

let insert(x,h) = merge(h, NODE(0,x,EMPTY,EMPTY))

let findMin h = match h with
| EMPTY -> raise EmptyHeap
| NODE(_,x,_,_) -> x

let deleteMin h = match h with
| EMPTY -> raise EmptyHeap
| NODE(_,x,lh,rh) -> merge(lh,rh)


let h1 = NODE(0, 40, EMPTY, EMPTY)
let h2 = NODE(0, 90, EMPTY, EMPTY)
let h3 = NODE(1, 20, h1, h2)
let h4 = NODE(0, 25, EMPTY, EMPTY)
let h5 = NODE(1, 8, h3, h4)
let h6 = NODE(0, 9, EMPTY, EMPTY)
let h7 = NODE(0, 13, EMPTY, EMPTY)
let h8 = NODE(1, 5, h6, h7)
let h9 = NODE(2, 4, h8, h5)

let h10 = NODE(0, 75, EMPTY, EMPTY)
let h11 = NODE(1, 22, h10, EMPTY)

let res = merge (h9, h11)

let rec string_of_heap: heap -> string = fun (heap) -> (
  match heap with
  | EMPTY -> "EMPTY"
  | NODE(_, v, l, r) -> (string_of_int v) ^ " -> " ^ "(" ^ (string_of_heap l) ^ ", " ^ (string_of_heap r) ^ ")"
)

let _ = print_endline (string_of_heap res)