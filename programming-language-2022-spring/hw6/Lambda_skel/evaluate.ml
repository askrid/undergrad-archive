(*
 * SNU 4190.310 Programming Languages (Spring 2022)
 *
 * Lambda Calculus
 *)
open Lambda
module Evaluator =
  struct
	exception Error of string

  module StrSet = Set.Make(struct type t = string let compare = Pervasives.compare end)

  type substitution = (string * lexp) list

  let rec new_var_helper n var_set = 
    let var_to_try = "#" ^ (string_of_int n) in
    if not (StrSet.mem var_to_try var_set) then
      var_to_try
    else
      new_var_helper (n + 1) var_set

  let new_var recommend var_set = 
    if not (StrSet.mem recommend var_set) then
      recommend
    else
      new_var_helper 0 var_set

  let rec free_vars = function
    | Var x -> StrSet.singleton x
    | Lam (x, e) -> StrSet.remove x (free_vars e)
    | App (e1, e2) -> StrSet.union (free_vars e1) (free_vars e2)

  let rec subs: (string * lexp) -> lexp -> lexp = fun (var, var_exp) exp ->
  match exp with
  | Var(x) -> if var = x then var_exp else exp
  | Lam(x,e) -> 
      let var_set = StrSet.singleton var in
      let var_set = StrSet.union var_set (free_vars var_exp) in
      let var_set = StrSet.union var_set (free_vars exp) in
      let x' = new_var x var_set in
      let e' = subs (x, Var x') e in
      let e'' = subs (var, var_exp) e' in
      Lam (x', e'')
  | App(e1,e2) -> App(subs (var, var_exp) e1, subs (var, var_exp) e2)

  let subst: substitution * lexp -> lexp = fun (sub, exp) ->
  List.fold_left (fun acc sub_ele -> subs sub_ele acc) exp sub  
  
  let rec reduce : lexp -> lexp = fun exp ->
    match exp with
    | Var _ -> exp
    | Lam (x, e) -> Lam (x, reduce(e))
    | App (e1, e2) -> (
      match e1 with
      | Var _ -> App (e1, reduce (e2))
      | Lam (x, e) -> reduce (subs (x, e2) e)
      | App (e11, e22) -> (
        let e1 = reduce (App (e11, e22)) in
        match e1 with
        | Var _ -> App (e1, reduce (e2))
        | Lam _ -> reduce (App (e1, e2))
        | App _ -> App (e1, reduce (e2))
      )
    )
end 