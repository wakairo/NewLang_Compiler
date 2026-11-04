## R1144 V2 pre-execution interface-calibrated FREEZE

2026-11-04 13:39:16.055 JST

C authorization: https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6622697245 。V1全bytes/oracle/NOT_RUNを保持し、V1のparse/CHECK/nativeを実測したとはしません。**V2はpost-freeze interface-calibrated cohort**です。P examples/fixtures/oracle/Issue/PR/reportは使用していません。

変更は `main -> unit` transportと、SUBの等価な `<` guard表記です。固定g+1は1/12/18でusizeに表現可能。各IDのcurrent列、Continue回数、Break値、外側Copy、actual、predicate branch、83/29選択はV1不変。最終scalarを新local `selected` で保持し、process main resultはunitと区別します。正例6・負例6の同じ観測質問であり新cohort増殖ではありません。FAST6／通常12 jobs／manual0、専用branch1・DRAFT PR1の予算は不変、現在すべて0。

全fileはUTF-8/ASCII、LF、末尾LF、BOMなし。fence内完全bytesのpure-JS SHA256（empty/abc既知vector照合済み）。localcompiler/native/build/test/formatter実行0。

| Path | bytes | LF count | SHA256 |
|---|---:|---:|---|
| tests/r1144/v2/sources/add_zero.nl | 397 | 20 | `acbb29df2e17efa0b136e624ea1582525d97a007a338d0855d9001beff8a0253` |
| tests/r1144/v2/sources/add_one.nl | 397 | 20 | `b053610361e1fb6a00c3d299cffbbc55b60869c3b2e8fdf60e93dfebf70f6b0b` |
| tests/r1144/v2/sources/add_three.nl | 396 | 20 | `dfb372c902fc2250ce101db6b717244c51bd2343b9c18bc6567effa1cb54b0fc` |
| tests/r1144/v2/sources/sub_zero.nl | 397 | 20 | `a2e76761c94723c4883d868aa6487a784730fd960f9bc746e89232e0b5345e4a` |
| tests/r1144/v2/sources/sub_one.nl | 397 | 20 | `4b99616022a880498fb8f657d4e3d6843f86b3d0b53a36e19e56c60be0280ca2` |
| tests/r1144/v2/sources/sub_three.nl | 397 | 20 | `726ec9baca80ace05db5740630b4e5c98be321e5bc6e884a03670dba3e04199a` |
| tests/r1144/v2/negative/underflow_guard.nl | 395 | 20 | `14a5780d154f8fa3804baf93fc472796fe005b4c2988c83deba599fd6833a9e7` |
| tests/r1144/v2/negative/carry_type.nl | 386 | 20 | `6a81101f12f5f20f34d83809ab23763dce28023b7d21992dd119eae8a7836cfd` |
| tests/r1144/v2/negative/carry_arity.nl | 394 | 20 | `246a55dc052edf8057af07f3e41154bde51e9e9dd397830fb3560e66b68790ed` |
| tests/r1144/v2/negative/result_actual_type.nl | 397 | 20 | `cb8770444ab2e442dab43b62111bb61de201ee0094a82c9a756a731c517b3812` |
| tests/r1144/v2/negative/scope_escape_name.nl | 398 | 20 | `08a1bb19fa9990b8aad1e4340ff67bd2bf10947935ec26ee83787a0f36a12870` |
| tests/r1144/v2/negative/even_sub_precision.nl | 395 | 20 | `cdfb42d1a08eb93abee6f8fe2db6c0d6522171ebacaba171a891611f52c52b75` |
| tests/r1144/oracle.v2.json | 10096 | 259 | `2e29cceb34c9e6fa323bbcb45629a076253aeb48241b0fad3d5259ef3f0e8176` |

### tests/r1144/v2/sources/add_zero.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(31)) {
        if (cursor < usize(31)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/sources/add_one.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/sources/add_three.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(5)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/sources/sub_zero.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(11)) {
        if (cursor < usize(12)) {
            break cursor;
        } else {
            continue(cursor - usize(3));
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/sources/sub_one.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(20)) {
        if (cursor < usize(18)) {
            break cursor;
        } else {
            continue(cursor - usize(3));
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/sources/sub_three.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(29)) {
        if (cursor < usize(18)) {
            break cursor;
        } else {
            continue(cursor - usize(4));
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/negative/underflow_guard.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(1)) {
        if (cursor < usize(1)) {
            break cursor;
        } else {
            continue(cursor - usize(2));
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/negative/carry_type.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(u8(17));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/negative/carry_arity.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor, cursor);
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/negative/result_actual_type.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break u8(17);
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/negative/scope_escape_name.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = cursor;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/v2/negative/even_sub_precision.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> unit {
    let after = loop (cursor = usize(2)) {
        if (cursor < usize(1)) {
            break cursor;
        } else {
            continue(cursor - usize(2));
        }
    };
    let mirrored = after;
    let selected = r1144_pick(mirrored);
    unit
}
```

### tests/r1144/oracle.v2.json

```json
{
  "schema": "r1144-v2",
  "authority": {
    "main": "acdbf91944a3bac99671d71c6ae23134b56d6c32",
    "spec": "Draft17.34",
    "p_head": "773643d2709a5e5db24650069d67c0d9032fefb0"
  },
  "independence": "V1 independently frozen before producer inspection; V2 is separate post-freeze interface-calibrated source, authorized by C comment 6622697245. No producer examples or oracle used.",
  "references": [
    "4.2",
    "5.2",
    "6",
    "11.1",
    "18.2",
    "18.5",
    "19.1",
    "27.1",
    "27.4",
    "27.5",
    "27.6",
    "27.7",
    "27.8"
  ],
  "byte_encoding": "UTF-8; LF; final LF; no BOM",
  "positive": [
    {
      "id": "A0",
      "path": "tests/r1144/v2/sources/add_zero.nl",
      "sha256": "acbb29df2e17efa0b136e624ea1582525d97a007a338d0855d9001beff8a0253",
      "expected_check": "accept",
      "expected_header_values": [
        31
      ],
      "expected_continue_count": 0,
      "expected_break": 31,
      "expected_outer_binding": 31,
      "expected_copy": 31,
      "expected_actual": 31,
      "expected_predicate_branch": "false",
      "expected_source_result": 29,
      "proof": "One usize carry, one Continue and one normal Break; finite literal-guard arithmetic preserves representability. Copy and call actual retain the executed Break value."
    },
    {
      "id": "A1",
      "path": "tests/r1144/v2/sources/add_one.nl",
      "sha256": "b053610361e1fb6a00c3d299cffbbc55b60869c3b2e8fdf60e93dfebf70f6b0b",
      "expected_check": "accept",
      "expected_header_values": [
        13,
        17
      ],
      "expected_continue_count": 1,
      "expected_break": 17,
      "expected_outer_binding": 17,
      "expected_copy": 17,
      "expected_actual": 17,
      "expected_predicate_branch": "true",
      "expected_source_result": 83,
      "proof": "One usize carry, one Continue and one normal Break; finite literal-guard arithmetic preserves representability. Copy and call actual retain the executed Break value."
    },
    {
      "id": "A3",
      "path": "tests/r1144/v2/sources/add_three.nl",
      "sha256": "dfb372c902fc2250ce101db6b717244c51bd2343b9c18bc6567effa1cb54b0fc",
      "expected_check": "accept",
      "expected_header_values": [
        5,
        9,
        13,
        17
      ],
      "expected_continue_count": 3,
      "expected_break": 17,
      "expected_outer_binding": 17,
      "expected_copy": 17,
      "expected_actual": 17,
      "expected_predicate_branch": "true",
      "expected_source_result": 83,
      "proof": "One usize carry, one Continue and one normal Break; finite literal-guard arithmetic preserves representability. Copy and call actual retain the executed Break value."
    },
    {
      "id": "S0",
      "path": "tests/r1144/v2/sources/sub_zero.nl",
      "sha256": "a2e76761c94723c4883d868aa6487a784730fd960f9bc746e89232e0b5345e4a",
      "expected_check": "accept",
      "expected_header_values": [
        11
      ],
      "expected_continue_count": 0,
      "expected_break": 11,
      "expected_outer_binding": 11,
      "expected_copy": 11,
      "expected_actual": 11,
      "expected_predicate_branch": "false",
      "expected_source_result": 29,
      "proof": "One usize carry, one Continue and one normal Break; finite literal-guard arithmetic preserves representability. Copy and call actual retain the executed Break value."
    },
    {
      "id": "S1",
      "path": "tests/r1144/v2/sources/sub_one.nl",
      "sha256": "4b99616022a880498fb8f657d4e3d6843f86b3d0b53a36e19e56c60be0280ca2",
      "expected_check": "accept",
      "expected_header_values": [
        20,
        17
      ],
      "expected_continue_count": 1,
      "expected_break": 17,
      "expected_outer_binding": 17,
      "expected_copy": 17,
      "expected_actual": 17,
      "expected_predicate_branch": "true",
      "expected_source_result": 83,
      "proof": "One usize carry, one Continue and one normal Break; finite literal-guard arithmetic preserves representability. Copy and call actual retain the executed Break value."
    },
    {
      "id": "S3",
      "path": "tests/r1144/v2/sources/sub_three.nl",
      "sha256": "726ec9baca80ace05db5740630b4e5c98be321e5bc6e884a03670dba3e04199a",
      "expected_check": "accept",
      "expected_header_values": [
        29,
        25,
        21,
        17
      ],
      "expected_continue_count": 3,
      "expected_break": 17,
      "expected_outer_binding": 17,
      "expected_copy": 17,
      "expected_actual": 17,
      "expected_predicate_branch": "true",
      "expected_source_result": 83,
      "proof": "One usize carry, one Continue and one normal Break; finite literal-guard arithmetic preserves representability. Copy and call actual retain the executed Break value."
    }
  ],
  "negative": [
    {
      "id": "N1",
      "path": "tests/r1144/v2/negative/underflow_guard.nl",
      "sha256": "14a5780d154f8fa3804baf93fc472796fe005b4c2988c83deba599fd6833a9e7",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "quantity precondition / CHECK",
      "reason": "At current=1 the true guard cursor>0 permits 1-2. usize cannot represent -1.",
      "native_allowed": false,
      "unsafe_if_executed": true
    },
    {
      "id": "N2",
      "path": "tests/r1144/v2/negative/carry_type.nl",
      "sha256": "6a81101f12f5f20f34d83809ab23763dce28023b7d21992dd119eae8a7836cfd",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "static carry type / frontend or CHECK",
      "reason": "Continue passes u8 into a usize parameter.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N3",
      "path": "tests/r1144/v2/negative/carry_arity.nl",
      "sha256": "246a55dc052edf8057af07f3e41154bde51e9e9dd397830fb3560e66b68790ed",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "static carry arity / frontend or CHECK",
      "reason": "Two Continue arguments target one parameter.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N4",
      "path": "tests/r1144/v2/negative/result_actual_type.nl",
      "sha256": "cb8770444ab2e442dab43b62111bb61de201ee0094a82c9a756a731c517b3812",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "ordinary call actual type / frontend or CHECK",
      "reason": "The loop result and its Copy are u8; r1144_pick requires usize. Break result need not equal carry type, so the actual mismatch is the intended invalidity.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N5",
      "path": "tests/r1144/v2/negative/scope_escape_name.nl",
      "sha256": "08a1bb19fa9990b8aad1e4340ff67bd2bf10947935ec26ee83787a0f36a12870",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "lexical name resolution / frontend or CHECK",
      "reason": "cursor is scoped to the loop body and cannot be named by the outer Copy binding.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N6",
      "path": "tests/r1144/v2/negative/even_sub_precision.nl",
      "sha256": "cdfb42d1a08eb93abee6f8fe2db6c0d6522171ebacaba171a891611f52c52b75",
      "classification": "canonical-legal-precision-rejection-permitted",
      "expected_check": "accept-or-analysis-precision-reject",
      "predicted_first_rejection_layer": "cyclic invariant/quantity precondition / CHECK",
      "reason": "Concrete states [2,0] are safe. A nonrelational guard-only lower bound of 1 does not prove subtraction by 2; conservative analysis may reject. This is not a language-invalid witness.",
      "native_allowed": false,
      "unsafe_if_executed": false,
      "concrete_header_values": [
        2,
        0
      ],
      "concrete_continue_count": 1,
      "concrete_break": 0,
      "concrete_source_result": 29
    }
  ],
  "observation": {
    "unmodified_guest": "Compile emitted C unchanged and record execution and output. Exit zero alone does not establish value observations.",
    "passive_observer": "A separate copy may add read-only output of existing guest values/control points. It must not compute or assign guest values, supply branch/loop outcomes, fabricate issuers, or substitute entry/constant for actual values.",
    "required_observed": [
      "header_values",
      "continue_count",
      "break_result",
      "outer_binding",
      "ordinary_copy",
      "callee_actual",
      "source_selected_result",
      "selected_binding"
    ],
    "required_stages": [
      "source/parser",
      "CHECK",
      "authentic-owned-if-applicable",
      "C17",
      "GCC-native",
      "Clang-native",
      "ASan",
      "UBSan"
    ],
    "unobserved_limits": [
      "Finite traces are not all-state proofs.",
      "No general range or termination proof assumed.",
      "Earlier rejection gives no evidence for later loan/barrier stages.",
      "Audit locality is UNMEASURED; GO624 PAUSED; H1 HOLD; P1036-PR1050 STOP."
    ]
  },
  "budget": {
    "positive_max": 6,
    "negative_max": 6,
    "fast_max_actual_runs": 6,
    "normal_max_actual_jobs": 12,
    "manual_rerun_or_dispatch": 0
  },
  "predecessor": {
    "oracle_path": "tests/r1144/oracle.v1.json",
    "sha256": "cb7d9db213332a6ccb7dd46ac3063039cab74fbfb236cc2bebebc6f700a637bd",
    "status": "NOT_RUN; known CLI/source-profile mismatch; no semantic or native measurement"
  },
  "transport": {
    "main_result": "unit",
    "observed_result": "Existing r1144_pick result is bound to selected and passively observed; expected_source_result retains the original scalar predicate-selected value.",
    "sub_guard": "cursor > usize(g) is written as if(cursor < usize(g+1)){break cursor;}else{continue(cursor-step);}. Fixed thresholds 1,12,18 are representable; no numerical/safety prediction changed."
  }
}
```

次は固定P/treeを親とするR-owned harness＋最小CTest登録の完成commit→初回ref直指し→FAST実確認へ進みます。V2のparser/CHECK/owned/C17/nativeも凍結時点ではNOT_RUNです。
