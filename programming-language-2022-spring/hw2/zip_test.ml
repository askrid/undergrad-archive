exception NOMOVE of string

type item = string
type tree = LEAF of item
| NODE of tree list

type zipper = TOP
| HAND of tree list * zipper * tree list

type location = LOC of tree * zipper

let goLeft loc = match loc with
| LOC(t, TOP) -> raise (NOMOVE "left of top")
| LOC(t, HAND(l::left, up, right)) -> LOC(l, HAND(left, up, t::right))
| LOC(t, HAND([], up, right)) -> raise (NOMOVE "left of first")

let goRight loc = match loc with
| LOC(t, TOP) -> raise (NOMOVE "right of top")
| LOC(t, HAND(left, up, r::right)) -> LOC(r, HAND(t::left, up, right))
| LOC(t, HAND(left, up, [])) -> raise (NOMOVE "right of first")

let goUp loc = match loc with
| LOC(t, TOP) -> raise (NOMOVE "up of top")
| LOC(t, HAND(left, up, right)) -> LOC(NODE(List.rev_append left (t::[]) @ right), up)

let goDown loc = match loc with
| LOC(LEAF _, _) -> raise (NOMOVE "down of bottom")
| LOC(NODE(t::right), curr) -> LOC(t, HAND([], curr, right))
| LOC(NODE([]), _) -> raise (NOMOVE "down of nowhere")


let rec string_of_tree tr = match tr with
| LEAF item -> item
| NODE (a::b::c::[]) -> "(" ^ (string_of_tree a) ^ ", " ^ (string_of_tree b) ^ ", " ^ (string_of_tree c) ^ ")"

let string_of_loc loc = match loc with
| LOC(t, _) -> string_of_tree t


let loc = LOC (LEAF "*",
HAND([LEAF "c"],
HAND([LEAF "+"; NODE [LEAF "a"; LEAF "*"; LEAF "b"]],
TOP,
[]),
[LEAF "d"]))

let loc = goUp loc
let loc = goUp loc
let loc = goDown loc
let loc = goRight loc
let loc = goRight loc
let loc = goDown loc
let loc = goRight loc
let loc = goRight loc
let loc = goLeft loc
let loc = goLeft loc
let loc = goUp loc
let loc = goUp loc

let _ = print_endline (string_of_loc loc)