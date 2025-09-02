module type Queue =
sig
  type element
  type queue
  exception EMPTY_Q
  val emptyQ: queue
  val enQ: queue * element -> queue
  val deQ: queue -> element * queue
end

module IntListQ =
struct
  type element = int list
  type queue = QUEUE of element list * element list

  exception EMPTY_Q

  let emptyQ = QUEUE ([], [])

  let enQ (q, el) = match q with
  | QUEUE(inbox, outbox) -> QUEUE(el::inbox, outbox)

  let deQ (q) = match q with
  | QUEUE([], []) -> raise EMPTY_Q
  | QUEUE(inbox, []) -> (
    let rev = List.rev inbox in
    let el::outbox = rev in
    (el, QUEUE([], outbox))
  )
  | QUEUE(inbox, el::outbox) -> (el, QUEUE(inbox, outbox))
end
