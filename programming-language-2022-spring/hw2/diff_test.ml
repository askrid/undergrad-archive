exception InvalidArgument

type ae = CONST of int
| VAR of string
| POWER of string * int
| TIMES of ae list
| SUM of ae list

let rec diff: ae * string -> ae = fun (expr, target) -> (
  match expr with
  | CONST i -> CONST 0
  | VAR s -> if (s = target) then (CONST 1) else (CONST 0)
  | POWER (s, i) -> if (s = target) then (
    match i with
    | 0 -> CONST 0
    | 1 -> CONST 1
    | i1 -> TIMES [CONST (i1); POWER (s, i1 - 1)]
  ) else (CONST 0)
  | TIMES [] -> raise InvalidArgument
  | TIMES [h] -> diff (h, target)
  | TIMES (h :: t) -> SUM [TIMES [diff (h, target); TIMES t]; TIMES [h; diff (TIMES t, target)]]
  | SUM [] -> raise InvalidArgument
  | SUM [h] -> diff (h, target)
  | SUM (h :: t) -> SUM [diff (h, target); diff ((SUM t), target)]
)

let rec string_of_ae: ae -> string = fun (expr) -> (
  match expr with
  | CONST i -> string_of_int i
  | VAR s -> s
  | POWER (s, i) -> (
    match i with
    | 0 -> "1"
    | 1 -> s 
    | i -> s ^ "^" ^ (string_of_int i)
  )
  | TIMES [] -> raise InvalidArgument
  | TIMES [h] -> string_of_ae h
  | TIMES (h :: t) -> (string_of_ae h) ^ " * " ^ (string_of_ae (TIMES t))
  | SUM [] -> raise InvalidArgument
  | SUM [h] -> string_of_ae h
  | SUM (h :: t) -> (string_of_ae h) ^ " + " ^ (string_of_ae (SUM t))
)

let expr = SUM [TIMES [VAR "a"; POWER ("x", 2)]; TIMES[VAR "b"; VAR "x"]; VAR "c"]
let _ = print_endline(string_of_ae expr)
let _ = print_endline(string_of_ae (diff (expr, "x")))
