# Coverage map

I = literal-origin ordinary usize identifier Copies to loop initializer; A = actual iteration phase Copies to helper actual; F = same-call fresh formal Copies to LT/EQ left operand. Counts exclude the normal argument receive and the initializer receive. Literal origin binding is link 0. The helper normal u8 and old-result u8 Copies are separate.

| Case | Class | I/A/F links | Purpose | selected index | Expected phases including Break |
|---|---|---|---|---:|---|
| C0 | direct control | 0/0/0 | direct baseline, LT branches | 1 | 2,5,8 |
| P1 | positive | 1/0/0 | one seed Copy; zero active trips | 0 | 42 |
| P2 | positive | 2/0/0 | two seed Copies; outer-name shadow; one backedge | 0 | 9,12 |
| P3 | positive | 0/2/0 | later phase must traverse two actual Copies | 1 | 4,7,10,13 |
| P4 | positive | 0/0/2 | per-call formal freshness; interior EQ true then false | 0 | 6,8,10,12,14 |
| P5 | positive | 0/1/1 | minimal actual+formal composition, same spelling across call | 1 | 1,3,5,7 |
| X1 | legal scope-out | 3/0/0 | initializer third-link limit | 1 | 3,5,7 |
| X2 | legal scope-out | 0/3/0 | actual third-link limit | 0 | 2,5,8 |
| X3 | legal scope-out | 0/0/3 | formal third-link limit | 1 | 1,3,5,7 |
| N1 | invalid | 0/u8/0 | Copy cannot convert u8 actual to usize formal | 1 | no legal runtime trace |
| N2 | invalid | 0/0/2 | usize condition is not bool | 0 | no legal runtime trace |
| N3 | invalid | 0/0/0 | Continue argument type must equal header type | 1 | no legal runtime trace |
| N4 | invalid | 0/0/0 | normal IF arm static types must agree | 0 | no legal runtime trace |

Budget: positive 5/6; ordinary direct control 1/1; invalid+legal scope-out 7/8. No boundary is gratuitously combined across all three axes. P5 is the only positive mixed-boundary case. LT and EQ, selected index 0/1, helper before/after main, reversed named initializer order, 0/1/multiple backedges are finite choices, not a cross-product claim.

Coverage not claimed: third-link support; outer guard/ADD carry/RHS bound aliases; dynamic initializer; arbitrary arithmetic; multiple helpers/carries/writers/loops; explicit helper RETURN; effects/ref/recursion; heap/nonCopy/dependency-bearing H; all public APIs; general loop soundness; private authority; audit locality.
