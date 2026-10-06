# P14 — bounded loop source / semantic contract

Track: P

正本は `docs/reference/CURRENT_SPEC.md` が指す Draft 17.17。主な対応箇所は
§19.1（terminating block item）、§21.8（ordinary names）、§27（loop / HYBRID
cyclic-state contract）。Backend Contract、merged P14-pre、F2 CLOSED は証拠であり、
仕様を上書きしない。この実装は LLVM / runtime loop lowering ではない。

## Source / receiving

exact surface は `loop (name = expression, ...) lexical_block`、
`continue(expression, ...);`、`break expression;`。
zero list は `loop ()` / `continue();`。parentheses と terminator semicolon は必須、
trailing comma / bare break / shorthand / control-item tail は拒否する。
loop expression、parameter initializer、Continue item、Break item は専用 syntax kind。
既存 flat syntax allocation ownership を使い、新しい subtree owner は追加しない。

ordinary-name classifier は `fn let return match if else loop continue break` を
structural reserved とする。`unit` は従来の distinguished-core diagnostic のまま。
field / variant / member label と near spelling は予約しない。

parameter 名の admissibility / distinctness を先に検査する。initializer は
left-to-right、すべて outer pre-loop namespace で検査し、先行 consume / effect は
後続 initializer に反映する。全 normal receiving が成功した後、body の独立 context
へ全 parameter binding を導入する。outer binding の shadowing は許す。
terminating initializer は既存 exit を伝播し、loop entry / unit を捏造しない。

## Control / ownership

`LoopControl` は synchronous lexical stack。nominal `NLControlTarget` は evidence が
retain する live handle であり、ValueId、scope ID、source text から推定しない。
block / IF / MATCH は nearest target を継承し、inner loop は shadow する。
ordinary callee は fresh Check / function target を持ち、caller loop を継承しない。
loan は従来どおり header-only API、callable source body は未導入であり、
それらを新たな loop-bearing source boundary として追加しない。

Continue / Break は current lexical edge を終える。Return は enclosing function
だけへ向かう。Loop は body の complete exit evidence を所有し、public parent へ
Continue / Break を持ち出さない。Return だけを function evidence へ伝播する。
`nl_checked_loop_body` で借用できる abstract body の semantic IDs は、その owned
context 専用である。checked loop は closure 成功後に `header_inductive` と各 edge
count を記録する。source lifetime は従来の artifact contract、registered body は
P11 durable body owner により維持する。

## Header H と inductiveness

cyclic slice の H は次の積として定める。

- carried flat Copy slot: exact static type、内容 / package number は unknown。
- carried non-Copy slot: entry の同じ flat nominal package、exactly one loose responsibility。
- captured outer non-Copy availability: entry と exact equality。
- outer places / current-value facts / scopes / domains / existing ref facts: exact entry frame。
- hidden dependency: dependency-free を証明できるものだけ。Unknown は free ではない。

`nl_loop_header_create_bounded(..., wide_slots=true, wide_memory=false)` は Entry inclusion
を検査する。body clone では flat Copy parameter を **entry の concrete value から
コピーせず、same-type unknown value** から bind する。non-Copy は証明済み shared
entry origin の package を移す。outer memory は exact invariant として読む。
この input で body 全体を一度 symbolic に検査し、IF の両 arm、MATCH の全 guarded
arm、sole-normal continuation から得られる **全** reachable Continue を供給する。
fixture の bool / variant だけを理由に Continue を落とさない。

各 Continue の operand は left-to-right ordinary use、exact arity/type、ownership、
captured availability、iteration-local nonescape を検査する。ended local tables を
projection した successor を entry と同じ nominal origin certificate で capture し、
`nl_loop_header_closure` がすべてに inclusion を課す。1本の代表 edge は選ばない。

したがって `Entry ⊑ H` と `ContinueTransfer_i(H) ⊑ H` を同時に検査する。
Copy-carried input が既に top なので widening iteration / least fixed point は不要。
outer current fact の変更は precision rejection になり、この成功条件が exact outer
frame を invariant にする。concrete first iteration の成功を later iteration へ
無条件に外挿する経路はない。rich entry が cyclic H constructor の範囲外でも、
Continue が **存在しない** finite loop は exact captured entry から検査できる。
この場合 recurrence closure は vacuous。rich finite entry の成功を cyclic success
と取り違えず、1本でも Continue があれば bounded H が必須。

Origin certificate は public value/current-fact prefix を共有する。fork 後 / body private
の同じ numeric ID は non-Copy identity にならない。new flat Copy package は内容を
unknown として抽象化できるが、reference / dependency / affine package はその方法で
抽象化しない。parameter receiving の local place identity 自体を backedge へ輸出しない。

## Projection / finite exit

existing block-obligation checker は Available non-Discardable local を拒否する。
implicit destroy は追加しない。Continue / Break は ending local place/scope への
surviving ref / dependency を検査し、ended binding/place/scope suffix だけを除去する。
value history と origin certificate は保持する。projection failure は public context を
変更しない。ptr / rich capability の cyclic correlation は precision fence に残る。

Break は header へ戻らない。全 Break の exact result type と captured non-Copy
availability を比較し、P13 frame / flat Copy current-fact join を再利用する。
flat Copy result は public unknown package（unit は package なし）。
same-origin non-Copy result は実際の public-prefix identity を保持する。
distinct / fresh-private non-Copy result を type だけから生成しない。
ref result は P7 の complete may-set を public stable origin / incarnation / scope /
provenance / access / occurrence facts へ rebase する。local capability は escape しない。
body-owned ID は直接 public result にしない。common affine consumption も全 Break の
証拠で確認して publish し、consumed package を public state に残さない。

zero Break は zero normal exit、type/result は absent。unit / never / fake Return は
作らない。Return があれば function exit evidence だけが残る。loop Return の bounded
result は flat Copy / unit、caller frame は unchanged を要求する。function boundary でも
全 early-loop Return と最終 result/frame を比較し、Copy result を unknown に join する。
Return-only loop は全 Return certificate が証明する function-local affine consumption
を反映する。Continue の post-state を function exit として採用しない。
有限 branch の divergence + real Return も zero-normal として区別し、flat Copy/unit と
exact caller frame / common local consumption の範囲で function result をまとめる。
empty exit set へ unit を追加する経路はない。

## Precision / resource / rollback

structured precision rejection（language error ではない）の対象:

- transformed non-Copy carry / carried ref / hidden dependency / private origin;
- cyclic outer Copy/current-memory change / rich raw or occurrence correlation;
- inner loop に Continue がある nested cyclic analysis;
- distinct non-Copy Break result、fresh private result、richer finite ref/frame join;
- non-flat loop Return result、caller effect / post-fork function-exit correlation;
- recursive body SCC / unsupported summary（既存 P11/P3 fence）。

finite inner Break loop は nearest-target shadowing / restoration を支持する。
複数 normal MATCH continuation は既存 structured precision fence。
不正 arity/type、captured availability mismatch、body fall-through は semantic diagnostic。

slot cap 16、exit cap 64、owned arm/body cap 64。既存 semantic entry/depth/body-call /
finite branch work budgets も継承する。resource exhaustion を success に変えない。
abstract transfer は1回で、反復回数による暗黙の convergence 判定はない。

parse / header / clone / evidence union / ref join / checked result の allocation failure は
従来の outer transaction 内で破棄する。public commit は成功時だけ。
新しい global mutable semantic state、runtime bitmap、CFG/SSA framework はない。

## 非対象

while/for、labels、mutable local syntax、recursive SCC、general nested fixpoint、
source-visible invariant、bool literal/operator、generic/associated/requires declaration、
callable redesign、LLVM、relocation、FFI、modules、concurrency、別 milestone は対象外。
