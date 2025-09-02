(*
 * SNU 4190.310 Programming Languages 
 * Homework "Continuation Passing Style" Skeleton
 *)

open Xexp

let count = ref 0

let new_name () = 
  let _ = count := !count + 1 in
  "x_" ^ (string_of_int !count)

let rec alpha_conv exp subst = 
  match exp with
  | Num n -> Num n
  | Var x -> (try Var (List.assoc x subst) with Not_found -> Var x)
  | Fn (x, e) ->
    let x' = new_name () in
    let subst' = (x, x') :: subst in
    Fn (x', alpha_conv e subst')
  | Fnr (f, x, e) -> 
    let x' = new_name () in
    let f' = new_name () in
    let subst' = (f, f') :: (x, x') :: subst in
    Fnr (f', x', alpha_conv e subst')
  | App (e1, e2) -> App (alpha_conv e1 subst, alpha_conv e2 subst)
  | Ifp (e1, e2, e3) -> 
    Ifp (alpha_conv e1 subst, alpha_conv e2 subst, alpha_conv e3 subst)
  | Add (e1, e2) -> Add (alpha_conv e1 subst, alpha_conv e2 subst)
  | Pair (e1, e2) -> Pair (alpha_conv e1 subst, alpha_conv e2 subst)
  | Fst e -> Fst (alpha_conv e subst)
  | Snd e -> Snd (alpha_conv e subst)
  | Raise e -> Raise (alpha_conv e subst)
  | Handle (e1, x, e2) ->
    let x' = new_name () in
    let subst' = (x, x') :: subst in
    Handle (alpha_conv e1 subst, x', alpha_conv e2 subst')

let rec xcps' exp = 
  let k_h = new_name() in
  let getk k_h = Fst (Var k_h) in
  let geth k_h = Snd (Var k_h) in
  let dapp (f, k, h) = App (f, Pair (k, h)) in

  match exp with
  (* Constant expressions *)
  | Num n -> Fn (k_h, App (getk k_h, Num n))
  | Var x -> Fn (k_h, App (getk k_h, Var x))
  | Fn (x, e) -> Fn (k_h, App (getk k_h, Fn (x, xcps' e)))
  | Fnr (f, x, e) -> Fn (k_h, App (getk k_h, Fnr (f, x, xcps' e)))

  (* Non constant expressions *)
  | App (e1, e2) -> 
    let f = new_name () in
    let v = new_name () in
    Fn (k_h, dapp (xcps' e1, Fn (f, dapp (xcps' e2, Fn (v, App (App (App (Var f, Var v), getk k_h), geth k_h)), geth k_h)), geth k_h))
  | Ifp (e1, e2, e3) ->
    let v1 = new_name () in
    let v2 = new_name () in
    let v3 = new_name () in
    Fn (k_h, dapp (xcps' e1,
      Fn (v1, Ifp (Var v1,
          dapp (xcps' e2, Fn (v2, App (getk k_h, Var v2)), geth k_h), dapp (xcps' e3, Fn (v3, App (getk k_h, Var v3)), geth k_h)
      )),
      geth k_h
      )
    )
  | Add (e1, e2) ->
    let v1 = new_name () in
    let v2 = new_name () in
    Fn (k_h, dapp (xcps' e1, Fn (v1, dapp (xcps' e2, Fn (v2, App (getk k_h, Add (Var v1, Var v2))), geth k_h)), geth k_h))
  | Pair (e1, e2) -> 
    let v1 = new_name () in
    let v2 = new_name () in
    Fn (k_h, dapp (xcps' e1, Fn (v1, dapp (xcps' e2, Fn (v2, App (getk k_h, Pair (Var v1, Var v2))), geth k_h)), geth k_h))
  | Fst e ->
    let v = new_name () in
    Fn (k_h, dapp (xcps' e, Fn (v, App (getk k_h, Fst (Var v))), geth k_h))
  | Snd e ->  
    let v = new_name () in
    Fn (k_h, dapp (xcps' e, Fn (v, App (getk k_h, Snd (Var v))), geth k_h))
  | Raise e -> Fn (k_h, dapp (xcps' e, geth k_h, geth k_h))
  | Handle (e1, x, e2) -> Fn (k_h, dapp (xcps' e1, getk k_h, Fn (x, dapp (xcps' e2, getk k_h, geth k_h))))

let xcps exp = xcps' (alpha_conv exp [])
