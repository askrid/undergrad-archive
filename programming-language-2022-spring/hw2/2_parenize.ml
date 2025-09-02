type team = Korea | France | Usa | Brazil | Japan | Nigeria | Cameroon
| Poland | Portugal | Italy | Germany | Norway | Sweden | England
| Argentina

type tourna = LEAF of team
| NODE of tourna * tourna

let team_to_string: team -> string = fun (tm) -> (
  match tm with
  | Korea -> "Korea"
  | France -> "France"
  | Usa -> "Usa"
  | Brazil -> "Brazil"
  | Japan -> "Japan"
  | Nigeria -> "Nigeria"
  | Cameroon -> "Cameroon"
  | Poland -> "Poland"
  | Portugal -> "Portugal"
  | Italy -> "Italy"
  | Germany -> "Germany"
  | Norway -> "Norway"
  | Sweden -> "Sweden"
  | England -> "England"
  | Argentina -> "Argentina"
)

let rec parenize: tourna -> string = fun (tna) -> (
  match tna with
  | LEAF tm -> (team_to_string tm)
  | NODE (tna1, tna2) -> ("(" ^ (parenize tna1) ^ " " ^ (parenize tna2) ^ ")")
)
