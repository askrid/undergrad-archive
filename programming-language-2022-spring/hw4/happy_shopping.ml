type require = id * (cond list)
and cond = Items of gift list
  | Same of id
  | Common of cond * cond
  | Except of cond * gift list
and gift = int
and id = A | B | C | D | E

let rec shoppingList: require list -> (id * gift list) list = fun (reqs) -> (
  let rslts = List.map (fun(req) -> (fst req, organize(requireToGiftList(req, [])))) reqs in
  let rslts_resolved = List.map (fun(req) -> (fst req, organize(requireToGiftList(req, rslts)))) reqs in

  let is_equal = List.for_all (fun(res_pair) -> fst res_pair = snd res_pair) (List.combine rslts rslts_resolved) in

  if is_equal then rslts_resolved else (
    let reqs_updated = List.map (fun(req) -> updateRequire(req, rslts_resolved)) reqs in
    shoppingList(reqs_updated)
  )
)

and requireToGiftList: require * (id * gift list) list -> gift list = fun(req, rslts) -> (
  let (name, cnds) = req in
  match cnds with
  | [] -> []
  | h::t -> condToGiftList(h, rslts) @ requireToGiftList((name, t), rslts)
)

and condToGiftList: cond * (id * gift list) list -> gift list = fun(cnd, rslts) -> (
  match cnd with
  | Items gfts -> gfts
  | Same name -> findGiftListOf(name, rslts)
  | Common (cnd1, cnd2) -> getCommon(cnd1, cnd2, rslts)
  | Except (cnd1, gfts) -> getExcept(cnd1, gfts, rslts)
)

and getCommon: cond * cond * (id * gift list) list -> gift list = fun(cnd1, cnd2, rslts) -> (
  let gfts1 = condToGiftList(cnd1, rslts) in
  let gfts2 = condToGiftList(cnd2, rslts) in
  match gfts1 with
  | [] -> []
  | h::t -> if (List.mem h gfts2) then h::getCommon(Items t, Items gfts2, rslts) else getCommon(Items t, Items gfts2, rslts)
)

and getExcept: cond * gift list * (id * gift list) list -> gift list = fun(cnd1, gfts2, rslts) -> (
  let gfts1 = condToGiftList(cnd1, rslts) in
  match gfts1 with
  | [] -> []
  | h::t -> if (List.mem h gfts2) then getExcept(Items t, gfts2, rslts) else h::getExcept(Items t, gfts2, rslts)
)

and removeDuplicates: gift list -> gift list = fun(gfts) -> (
  match gfts with
  | [] -> []
  | h::t -> if (List.mem h t) then removeDuplicates(t) else h::removeDuplicates(t)
)

and organize: gift list -> gift list = fun(gfts) -> List.sort compare (removeDuplicates(gfts))

and findGiftListOf: id * (id * gift list) list -> gift list = fun(name, rslts) -> (
  try
    snd (List.find (fun(res) -> (fst res) = name) rslts)
  with Not_found -> []
)

and updateRequire: require * (id * gift list) list -> require = fun(req, rslts) -> (
  let name = fst req in
  let cnds = snd req in
  let gfts = findGiftListOf(name, rslts) in
  (name, (Items gfts)::cnds)
)