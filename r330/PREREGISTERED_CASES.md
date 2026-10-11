# R330 preregistered fourth-direct-change checked-owned artifact consistency audit
Track: R (Independent Red Team). NON-BLIND, test-only, immutable compiler PR326 de9c4b175784f484675b670a109f62edeaf35de1.
Canonical Compiler/main a5343bf70004b30a8b026e52aa2fde93b5920be0, Draft17.30. No P329 unpublished output.
This is the **initial manifest locked before the first R330 CI experiment**. Every case below is an independently specified bounded in-memory modification of the actual source-generated checked-owned artifact. NewLang input is unchanged R328 original own 184-line calibrated source SHA256 b6033c21e2b86d72f5a352326a9874e0c786a0774bc4a8262f44bc6feaef51c4. This audit does not newly establish source success; R328 calibrated it. No manually synthesized positive worlds. All success and refusal oracle calls use the same 5 public captured-closure and 2 public scoped-field-call validators. Private semantic_internal.h is used solely to make controlled in-bounds negative mutations on the genuine allocated object. Each test captures object bytes, mutates, calls 7 public validators, restores the exact saved bytes, and revalidates 7/7 status0. No cross-run state leaking. Status3 means public refusal (precision), NOT invalid-source semantic rejection.

| id | intended contradictory invariant | controlled negative edit in existing object |
|---|---|---|
| 00 | Eighth evidence absent | decrease final caller field_change_count from 8 to 7 |
| 01 | Eighth event falsely reused seventh operation | fourth evidence node = third node |
| 02 | Source node chronology inverted | exchange third/fourth evidence node identifiers in paired edits |
| 03 | Eighth replacement missing | direct C.prev node kind = ordinary statement |
| 04 | Eighth source projection claims A root | operation field.parent changed C→A |
| 05 | Eighth source projection claims C.next sibling | operation field.child changed C.prev→C.next |
| 06 | Eighth source field index wrong | operation field.index changed prev(1)→next(0) |
| 07 | Old value mislabeled as new | operation field.old_value = field.new_value |
| 08 | New value mislabeled as old | operation field.new_value = field.old_value |
| 09 | Pre-fourth C physical region false | C root placement.region changed 4→2 before |
| 10 | Pre-fourth C governing Domain false | C root governing_domain changed 4→2 before |
| 11 | Post-fourth C governing Domain false | C root governing_domain changed 4→2 after |
| 12 | Pre-fourth C.prev current fact contradictory | field C.prev current_fact incremented only before |
| 13 | Fourth C.prev does not create new current fact | after C.prev current_fact set to before value |
| 14 | Old Some(B) incorrectly points C | before old Option pointer payload target B→C |
| 15 | New Some(A) incorrectly points B | before new Option pointer payload target A→B |
| 16 | Old Some(B) occurrence not ended | after old occurrence live=true |
| 17 | New Some(A) occurrence not live | after fresh occurrence live=false |
| 18 | New C.prev field references retired occurrence | after C.prev payload_occurrence=old occurrence |
| 19 | Paired bogus predecessor/receiver B currentness | third.after B root and fourth.before B root current_fact incremented together |
| 20 | C source write loan spuriously survives | final owned context C projection scope marked active |
| 21 | C sibling illegally changes at fourth Change | after C.next sibling current_fact incremented |
| 22 | Fourth before anchor shifted to earlier (valid) third-before checkpoint | fourth.before and fourth.before_origin both rebound to third.before |

All controls selected BEFORE first R330 CI job; if cases are later extended, identify them separately as second-stage (no retroactive preregistration). If an edit cannot be safely located or creates no actual bytes change, observer must HOLD/fail, not claim a valid trial. Expected meaningful contradiction: >=1 published validator nonzero, then after full restore all seven exactly status0; any all-seven-OK invalid mutant fails the job and must be investigated, not suppressed. No P319 case code copied or reexecuted; overlaps in abstract invariants with P319/R322 are disclosed, not claimed as new defect families. No compiler or PR326 edits/merge, no draft spec change, no P329 read.
