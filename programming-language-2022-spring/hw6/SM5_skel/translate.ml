(*
 * SNU 4190.310 Programming Languages 
 * K-- to SM5 translator skeleton code
 *)

open K
open Sm5
module Translator = struct
  let rec trans : K.program -> Sm5.command = fun p ->
    match p with
    | K.NUM i -> [Sm5.PUSH (Sm5.Val (Sm5.Z i))]
    | K.TRUE -> [Sm5.PUSH (Sm5.Val (Sm5.B true))]
    | K.FALSE -> [Sm5.PUSH (Sm5.Val (Sm5.B false))]
    | K.UNIT -> [Sm5.PUSH (Sm5.Val (Sm5.Unit))]
    | K.VAR x -> [Sm5.PUSH (Sm5.Id x); Sm5.LOAD]
    | K.ADD (e1, e2) -> trans e1 @ trans e2 @ [Sm5.ADD]
    | K.SUB (e1, e2) -> trans e1 @ trans e2 @ [Sm5.SUB]
    | K.MUL (e1, e2) -> trans e1 @ trans e2 @ [Sm5.MUL]
    | K.DIV (e1, e2) -> trans e1 @ trans e2 @ [Sm5.DIV]
    | K.EQUAL (e1, e2) -> trans e1 @ trans e2 @ [Sm5.EQ]
    | K.LESS (e1, e2) -> trans e1 @ trans e2 @ [Sm5.LESS]
    | K.NOT e -> trans e @ [Sm5.NOT]
    | K.READ x -> [Sm5.GET; Sm5.PUSH (Sm5.Id x); Sm5.STORE; Sm5.PUSH (Sm5.Id x); Sm5.LOAD]
    | K.WRITE e -> trans e @ [Sm5.PUT] @ trans e
    | K.LETV (x, e1, e2) ->
      trans e1 @ [Sm5.MALLOC; Sm5.BIND x; Sm5.PUSH (Sm5.Id x); Sm5.STORE] @
      trans e2 @ [Sm5.UNBIND; Sm5.POP]
    | K.LETF (f, x, e1, e2) ->
      [Sm5.PUSH (Sm5.Fn (x, [Sm5.BIND f] @ trans e1)); Sm5.BIND f] @
      trans e2 @ [Sm5.UNBIND; Sm5.POP]
    | K.ASSIGN (x, e) -> trans e @ [Sm5.PUSH (Sm5.Id x); Sm5.STORE; Sm5.PUSH (Sm5.Id x); Sm5.LOAD]
    | K.IF (e_cond ,e_true, e_false) -> trans e_cond @ [Sm5.JTR (trans e_true, trans e_false)]
    | K.WHILE (e_cond, e_body) -> trans (desugar_while (e_cond, e_body))
    | K.FOR (x, e1, e2, e_body) -> trans (desugar_for (x, e1, e2, e_body))
    | K.SEQ (e1, e2) -> trans e1 @ [Sm5.POP] @ trans e2
    | K.CALLV (f, e) ->
      [Sm5.PUSH (Sm5.Id f); Sm5.PUSH (Sm5.Id f)] @ trans e @ [Sm5.MALLOC; Sm5.CALL]
    | K.CALLR (f, x) ->
      [Sm5.PUSH (Sm5.Id f); Sm5.PUSH (Sm5.Id f); Sm5.PUSH (Sm5.Id x); Sm5.LOAD; Sm5.PUSH (Sm5.Id x); Sm5.CALL]

  and desugar_while : K.exp * K.exp -> K.program = fun (e_cond, e_body) ->
    let fun_name = "f" in
    let arg_name = "x" in
    K.LETF(
      fun_name,
      arg_name,
      K.IF(
        e_cond,
        K.SEQ(
          e_body,
          K.CALLV(fun_name, K.UNIT)
        ),
        K.UNIT
      ),
      K.CALLV(fun_name, K.UNIT)
    )

  and desugar_for : K.id * K.exp * K.exp * K.exp -> K.program = fun (x, e1, e2, e_body) ->
    K.IF(
      K.LESS(e2, e1),
      K.UNIT,
      K.SEQ(
        K.ASSIGN(x, e1),
        K.SEQ(
          e_body,
          K.WHILE(
            K.LESS(K.VAR x, e2),
            K.SEQ(
              K.ASSIGN(x, K.ADD(K.VAR x, K.NUM 1)),
              e_body
            )
          )
        )
      )
    )

end
