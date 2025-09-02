(*
 * SNU 4190.310 Programming Languages 
 * Homework "RozettaX" Skeleton
 *)

let trans_v : Sm5.value -> Sonata.value = function
  | Sm5.Z z  -> Sonata.Z z
  | Sm5.B b  -> Sonata.B b
  | Sm5.L _ -> raise (Sonata.Error "Invalid input program : pushing location")
  | Sm5.Unit -> Sonata.Unit
  | Sm5.R _ -> raise (Sonata.Error "Invalid input program : pushing record")

let rec trans_obj : Sm5.obj -> Sonata.obj = function
  | Sm5.Val v -> Sonata.Val (trans_v v)
  | Sm5.Id id -> Sonata.Id id
  | Sm5.Fn (arg, command) ->
    let r = "@ra" in
    let command' = 
      trans' (
        [Sm5.BIND r] @
        command @
        [Sm5.PUSH (Sm5.Id r); Sm5.UNBIND; Sm5.POP] @
        [Sm5.PUSH (Sm5.Val Sm5.Unit)] @
        [Sm5.MALLOC] @
        [Sm5.CALL]
      )
    in
    Sonata.Fn (arg, command')

and trans' : Sm5.command -> Sonata.command = function
  | Sm5.PUSH obj :: cmds -> Sonata.PUSH (trans_obj obj) :: (trans' cmds)
  | Sm5.POP :: cmds -> Sonata.POP :: (trans' cmds)
  | Sm5.STORE :: cmds -> Sonata.STORE :: (trans' cmds)
  | Sm5.LOAD :: cmds -> Sonata.LOAD :: (trans' cmds)
  | Sm5.MALLOC :: cmds -> Sonata.MALLOC :: (trans' cmds)
  | Sm5.BOX z :: cmds -> Sonata.BOX z :: (trans' cmds)
  | Sm5.UNBOX id :: cmds -> Sonata.UNBOX id :: (trans' cmds)
  | Sm5.BIND id :: cmds -> Sonata.BIND id :: (trans' cmds)
  | Sm5.UNBIND :: cmds -> Sonata.UNBIND :: (trans' cmds)
  | Sm5.GET ::cmds -> Sonata.GET :: (trans' cmds)
  | Sm5.PUT ::cmds -> Sonata.PUT :: (trans' cmds)
  | Sm5.ADD :: cmds -> Sonata.ADD :: (trans' cmds)
  | Sm5.SUB :: cmds -> Sonata.SUB :: (trans' cmds)
  | Sm5.MUL :: cmds -> Sonata.MUL :: (trans' cmds)
  | Sm5.DIV :: cmds -> Sonata.DIV :: (trans' cmds)
  | Sm5.EQ :: cmds -> Sonata.EQ :: (trans' cmds)
  | Sm5.LESS :: cmds -> Sonata.LESS :: (trans' cmds)
  | Sm5.NOT :: cmds -> Sonata.NOT :: (trans' cmds)
  | Sm5.JTR (c1, c2) :: cmds -> [Sonata.JTR (trans' (c1 @ cmds), trans' (c2 @ cmds))]
  | Sm5.CALL :: [] -> [Sonata.CALL]
  | Sm5.CALL :: cmds -> 
    let l = "@loc" in
    let v = "@val" in
    let p = "@proc" in
    let a = "@arg" in
    let cont = Sonata.Fn (a, trans' cmds) in
    [Sonata.BIND l] @
    [Sonata.MALLOC; Sonata.BIND v; Sonata.PUSH (Sonata.Id v); Sonata.STORE] @
    [Sonata.BIND p] @
    [Sonata.PUSH cont] @
    [Sonata.PUSH (Sonata.Id p); Sonata.UNBIND; Sonata.POP] @
    [Sonata.PUSH (Sonata.Id v); Sonata.LOAD; Sonata.UNBIND; Sonata.POP] @
    [Sonata.PUSH (Sonata.Id l); Sonata.UNBIND; Sonata.POP] @
    [Sonata.CALL]
  | [] -> []

let trans : Sm5.command -> Sonata.command = fun command -> trans' command