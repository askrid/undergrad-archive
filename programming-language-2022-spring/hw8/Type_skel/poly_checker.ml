(*
 * SNU 4190.310 Programming Languages
 * Type Checker Skeleton
 *)

open M

type var = string

type typ = 
  | TInt
  | TBool
  | TString
  | TPair of typ * typ
  | TLoc of typ
  | TFun of typ * typ
  | TVar of var
  | TWritable of var

type typ_scheme =
  | SimpleTyp of typ 
  | GenTyp of (var list * typ)

type typ_env = (M.id * typ_scheme) list

let count = ref 0 

let new_var () = 
  let _ = count := !count +1 in
  "x_" ^ (string_of_int !count)

(* Definitions related to free type variable *)

let union_ftv ftv_1 ftv_2 = 
  let ftv_1' = List.filter (fun v -> not (List.mem v ftv_2)) ftv_1 in
  ftv_1' @ ftv_2
  
let sub_ftv ftv_1 ftv_2 =
  List.filter (fun v -> not (List.mem v ftv_2)) ftv_1

let rec ftv_of_typ : typ -> var list = function
  | TInt | TBool | TString -> []
  | TPair (t1, t2) -> union_ftv (ftv_of_typ t1) (ftv_of_typ t2)
  | TLoc t -> ftv_of_typ t
  | TFun (t1, t2) ->  union_ftv (ftv_of_typ t1) (ftv_of_typ t2)
  | TVar v | TWritable v -> [v]

let ftv_of_scheme : typ_scheme -> var list = function
  | SimpleTyp t -> ftv_of_typ t
  | GenTyp (alphas, t) -> sub_ftv (ftv_of_typ t) alphas 

let ftv_of_env : typ_env -> var list = fun tyenv ->
  List.fold_left 
    (fun acc_ftv (id, tyscm) -> union_ftv acc_ftv (ftv_of_scheme tyscm))
    [] tyenv 

(* Generalize given type into a type scheme *)
let generalize : typ_env -> typ -> typ_scheme = fun tyenv t ->
  let env_ftv = ftv_of_env tyenv in
  let typ_ftv = ftv_of_typ t in
  let ftv = sub_ftv typ_ftv env_ftv in
  if List.length ftv = 0 then
    SimpleTyp t
  else
    GenTyp(ftv, t)

(* Definitions related to substitution *)

type subst = typ -> typ

let empty_subst : subst = fun t -> t

let make_subst : var -> typ -> subst = fun x t ->
  let rec subs t' = 
    match t' with
    | TVar x' -> if (x = x') then t else t'
    | TPair (t1, t2) -> TPair (subs t1, subs t2)
    | TLoc t'' -> TLoc (subs t'')
    | TFun (t1, t2) -> TFun (subs t1, subs t2)
    | TInt | TBool | TString -> t'
    | TWritable x' -> if (x = x') then (
      match t with
      | TVar x'' -> TWritable x''
      | _ -> t
    ) else t'
  in subs

let (@@) s1 s2 = (fun t -> s1 (s2 t)) (* substitution composition *)

let subst_scheme : subst -> typ_scheme -> typ_scheme = fun subs tyscm ->
  match tyscm with
  | SimpleTyp t -> SimpleTyp (subs t)
  | GenTyp (alphas, t) ->
    (* S (\all a.t) = \all b.S{a->b}t  (where b is new variable) *)
    let betas = List.map (fun _ -> new_var()) alphas in
    let s' =
      List.fold_left2
        (fun acc_subst alpha beta -> make_subst alpha (TVar beta) @@ acc_subst)
        empty_subst alphas betas
    in
    GenTyp (betas, subs (s' t))

let subst_env : subst -> typ_env -> typ_env = fun subs tyenv ->
  List.map (fun (x, tyscm) -> (x, subst_scheme subs tyscm)) tyenv


(* Convert poly_checker typ to M.typ *)

let rec mtyp : typ -> M.typ = fun t ->
  match t with
  | TInt -> M.TyInt
  | TBool -> M.TyBool
  | TString -> M.TyString
  | TPair (l, r) -> M.TyPair (mtyp l, mtyp r)
  | TLoc t -> M.TyLoc (mtyp t)
  | _ -> raise (M.TypeError "TypeError in m_typ()") 

(* Definitions related to unification *)

let rec occurs : var -> typ -> bool = fun x t -> 
  match t with
  | TInt | TBool | TString -> false
  | TPair (l, r)
  | TFun (l, r) -> occurs x l || occurs x r
  | TLoc l -> occurs x l
  | TVar n | TWritable n -> x = n

let rec unify : typ -> typ -> subst = fun left right ->
  if (left = right) then empty_subst else
    match (left, right) with
    | TVar alpha, t when not (occurs alpha t) -> make_subst alpha t
    | t, TVar alpha -> unify (TVar alpha) t
    | TWritable alpha, t when not (occurs alpha t) -> (
      match t with
      | TInt | TBool | TString | TWritable _ -> make_subst alpha t
      | _ -> raise (M.TypeError "Unwritable type")
    )
    | t, TWritable alpha -> unify (TWritable alpha) t
    | TLoc l, TLoc r -> unify l r
    | TPair (ll, lr), TPair(rl, rr)
    | TFun (ll, lr), TFun (rl, rr) -> 
      let s1 = unify ll rl in
      let s2 = unify (s1 lr) (s1 rr) in
      s2 @@ s1
    | _ -> raise (M.TypeError "TypeError in unify()")

(* Check if the M expresion is expansive *)

let rec expansive : M.exp -> bool = fun e ->
  match e with
  | M.CONST _ | M.VAR _ | M.FN _ | M.READ -> false
  | M.MALLOC _ | M.APP _ -> true
  | M.WRITE e | M.BANG e | M.FST e | M.SND e -> expansive e
  | M.LET (M.REC (_, _, e1), e2) | M.LET (M.VAL (_, e1), e2)
  | M.BOP (_, e1, e2) | M.ASSIGN (e1, e2) | M.SEQ (e1, e2) | M.PAIR (e1, e2) ->
    expansive e1 || expansive e2
  | M.IF (c, t, f) -> expansive c || expansive t || expansive f

(* M algorithm implementation *)

let rec m_algo : typ_env -> M.exp -> typ -> subst = fun tenv texp ttyp ->
  match texp with
  | M.CONST M.S _ -> unify ttyp TString
  | M.CONST M.N _ -> unify ttyp TInt
  | M.CONST M.B _ -> unify ttyp TBool
  | M.VAR x ->
    let ts = try List.assoc x tenv with | Not_found -> raise (M.TypeError ("TypeError in m_algo()")) in
    let ts = subst_scheme empty_subst ts in
    let t = (match ts with
    | SimpleTyp t -> t
    | GenTyp (_, t) -> t) in
    unify ttyp t
  | M.FN (x, e) ->
    let b1 = TVar (new_var()) in
    let b2 = TVar (new_var()) in
    let s1 = unify ttyp (TFun (b1, b2)) in
    let tenv = subst_env s1 tenv in
    let tenv = tenv @ [(x, SimpleTyp (s1 b1))] in
    let s2 = m_algo tenv e (s1 b2) in
    s2 @@ s1
  | M.APP (e1, e2) ->
    let b = TVar (new_var()) in
    let s1 = m_algo tenv e1 (TFun (b, ttyp)) in
    let s2 = m_algo (subst_env s1 tenv) e2 (s1 b) in
    s2 @@ s1
  | M.LET (M.VAL (x, e1), e2) ->
    let b = TVar (new_var()) in
    let s1 = m_algo tenv e1 b in
    let b = s1 b in
    let tenv = subst_env s1 tenv in
    let ts = if (expansive e1) then SimpleTyp b else generalize tenv b in
    let s2 = m_algo (tenv @ [(x, ts)]) e2 (s1 ttyp) in
    s2 @@ s1
  | M.LET (M.REC (f, x, e1), e2) ->
    let b = TVar (new_var()) in
    let s1 = m_algo (tenv @ [(f, SimpleTyp b)]) (M.FN (x, e1)) b in
    let b = s1 b in
    let s2 = m_algo ((subst_env s1 tenv) @ [f, generalize tenv b]) e2 (s1 ttyp) in
    s2 @@ s1
  | M.READ -> unify ttyp TInt
  | M.WRITE e ->
    let s1 = unify ttyp (TWritable (new_var())) in
    let s2 = m_algo (subst_env s1 tenv) e (s1 ttyp) in
    s2 @@ s1
  | M.PAIR (e1, e2) -> 
    let b1 = TVar (new_var ()) in
    let b2 = TVar (new_var ()) in
    let s1 = unify ttyp (TPair (b1, b2)) in
    let b1 = s1 b1 in
    let b2 = s1 b2 in
    let tenv = subst_env s1 tenv in
    let s2 = m_algo tenv e1 b1 in
    let b2 = s2 b2 in
    let tenv = subst_env s2 tenv in
    let s3 = m_algo tenv e2 b2 in
    s3 @@ s2 @@ s1
  | M.FST e -> m_algo tenv e (TPair (ttyp, TVar (new_var ())))
  | M.SND e -> m_algo tenv e (TPair (TVar (new_var ()), ttyp))
  | M.BANG e -> m_algo tenv e (TLoc ttyp)
  | M.SEQ (e1, e2) ->
    let b = TVar (new_var()) in
    let s1 = m_algo tenv e1 b in
    let s2 = m_algo (subst_env s1 tenv) e2 (s1 ttyp) in
    s2 @@ s1
  | M.ASSIGN (e1, e2) ->
    let s1 = m_algo tenv e1 (TLoc ttyp) in
    let s2 = m_algo (subst_env s1 tenv) e2 (s1 ttyp) in
    s2 @@ s1
  | M.MALLOC e ->
    let b = TVar (new_var()) in
    let s1 = m_algo tenv e b in
    let s2 = unify (s1 ttyp) (TLoc (s1 b)) in
    s2 @@ s1
  | M.IF (c, e1, e2) ->
    let s1 = m_algo tenv c TBool in
    let tenv = subst_env s1 tenv in
    let ttyp = s1 ttyp in
    let s2 = m_algo tenv e1 ttyp in
    let tenv = subst_env s2 tenv in
    let ttyp = s2 ttyp in
    let s3 = m_algo tenv e2 ttyp in
    s3 @@ s2 @@ s1
  | M.BOP (op, e1, e2) ->
    let foo : typ -> typ -> subst = fun t1 t2 ->
      let s1 = unify ttyp t2 in
      let t1 = s1 t1 in
      let tenv = subst_env s1 tenv in
      let s2 = m_algo tenv e1 t1 in
      let t1 = s2 t1 in
      let tenv = subst_env s2 tenv in
      let s3 = m_algo tenv e2 t1 in
      s3 @@ s2 @@ s1
    in
    match op with
    | M.ADD | M.SUB -> foo TInt TInt
    | M.AND | M.OR -> foo TBool TBool
    | M.EQ -> foo (TVar (new_var())) TBool

let check : M.exp -> M.typ = fun e ->
  let a = TVar (new_var ()) in
  let s = m_algo [] e a in
  mtyp (s a)