## R1144 V1 independent source / negative / oracle FREEZE

凍結時刻: 2026-11-04 13:33:40.668 JST

**実行前、producer inspection前。** 初期資料はIssue #1144。固定mainのCompass→Value Strategy §§1.1/1.3、Process §§7/8、CURRENT_SPEC→Draft17.34 §§4.2,5.2,6,11.1,18.2,18.5,19.1,27.1,27.4–27.8から独立に作成。P実装・P Issue/PR本文・tests/fixtures/oracle・報告、M/F/B具体例、新旧R入力/oracle/rawは未閲覧・未使用。CURRENT_SPECのpointer付随historyにはリンクの記載がありましたが、その先の具体成果は未閲覧です。

正例6はADD/SUB各zero/one/three Continue、正常Break結果31/17/11を外側binding→Copy→usize actualへ渡し、callee内のsource ifが83/29を選びます。entry値への取り違え・一律17/83への置換を区別します。予測するcurrent列・Continue回数・Break/Copy/actual・最終結果はoracle内で固定しています。

負例6はunderflow guard、Continue型、Continue arity、結果のactual型、loop名scope、canonical-legalなeven-step precision probe。N4では**carry型とBreak型が異なること自体を違反としません**。usize calleeへu8 actualを渡す点が違反です。N6は合法な具体列[2,0]でありprecision拒否を許容し、他のcanonical-invalidと区別します。負例はすべてnative禁止。overflow固有例とref dependencyの動的実測は今回未観測です。

byte rule: 全fileはUTF-8、ASCII文字のみ、LF、末尾LFあり、BOMなし。以下の各fence内部の最初の文字から閉じfence直前のLFまでが完全bytesです。SHA256はデータ文字列からpure JavaScript実装で算出し、emptyとabcの既知vectorで照合。local compiler/native/build/test/formatter実行0。

| Path | bytes | LF count | SHA256 |
|---|---:|---:|---|
| tests/r1144/sources/add_zero.nl | 370 | 19 | `2f99b12fea9bf70d82c27bafe2153ea43af17813e5e1f16e7c5acb0ce877a87e` |
| tests/r1144/sources/add_one.nl | 370 | 19 | `d7967b1c934d38594d7c7075b5042518b226e6a2463b1af5f35df931c68cceac` |
| tests/r1144/sources/add_three.nl | 369 | 19 | `71d1681752e3f363a30ede801f6a5463c0192b741f32508f05c2f269e4daf110` |
| tests/r1144/sources/sub_zero.nl | 370 | 19 | `c941016972d316d096187cf6dfe56e099c6d4c2ca91d026fc0f8d053b5677483` |
| tests/r1144/sources/sub_one.nl | 370 | 19 | `936e5fe76a882374dc6e2744840086a1eda9151f642c44a4e3f1684bdb133532` |
| tests/r1144/sources/sub_three.nl | 370 | 19 | `f5aa6f52ac1b93731acdb8ed4882e96daa9550ad78161e4ff0b75885f7dbb21f` |
| tests/r1144/negative/underflow_guard.nl | 368 | 19 | `70e7be8790dc5fa411989d17e6c86b9d8629603daa7f0502de4cf886f64cbb62` |
| tests/r1144/negative/carry_type.nl | 359 | 19 | `7463dfb0f13968f9cf8f859887781d2c0f2a7d6d44d8c1cdcb1a09900a0d3ec9` |
| tests/r1144/negative/carry_arity.nl | 367 | 19 | `82cc85ac62e715c485725b0e168fab78a43ee43e8e9fed3b1486e6b4235305d2` |
| tests/r1144/negative/result_actual_type.nl | 370 | 19 | `7c8fab78db90e6e9b4e728e3b32c0e06ed5d823c6d2a34f59628e75153b85a91` |
| tests/r1144/negative/scope_escape_name.nl | 371 | 19 | `25160bf8624b77263c51572e83297c3279a16b1b0be6c8a27ca44802cacbc722` |
| tests/r1144/negative/even_sub_precision.nl | 368 | 19 | `0d892f7154ca367a8ee0747aac1e655ec0b6558ae56b7286f45e56a97b7d2b7e` |
| tests/r1144/oracle.v1.json | 9280 | 248 | `cb7d9db213332a6ccb7dd46ac3063039cab74fbfb236cc2bebebc6f700a637bd` |

### tests/r1144/sources/add_zero.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(31)) {
        if (cursor < usize(31)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/sources/add_one.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/sources/add_three.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(5)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/sources/sub_zero.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(11)) {
        if (cursor > usize(11)) {
            continue(cursor - usize(3));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/sources/sub_one.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(20)) {
        if (cursor > usize(17)) {
            continue(cursor - usize(3));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/sources/sub_three.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(29)) {
        if (cursor > usize(17)) {
            continue(cursor - usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/negative/underflow_guard.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(1)) {
        if (cursor > usize(0)) {
            continue(cursor - usize(2));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/negative/carry_type.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(u8(17));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/negative/carry_arity.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor, cursor);
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/negative/result_actual_type.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break u8(17);
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/negative/scope_escape_name.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(13)) {
        if (cursor < usize(17)) {
            continue(cursor + usize(4));
        } else {
            break cursor;
        }
    };
    let mirrored = cursor;
    r1144_pick(mirrored)
}
```

### tests/r1144/negative/even_sub_precision.nl

```text
fn r1144_pick(amount: usize) -> u8 {
    if (amount == usize(17)) {
        u8(83)
    } else {
        u8(29)
    }
}

fn main() -> u8 {
    let after = loop (cursor = usize(2)) {
        if (cursor > usize(0)) {
            continue(cursor - usize(2));
        } else {
            break cursor;
        }
    };
    let mirrored = after;
    r1144_pick(mirrored)
}
```

### tests/r1144/oracle.v1.json

```json
{
  "schema": "r1144-v1",
  "authority": {
    "main": "acdbf91944a3bac99671d71c6ae23134b56d6c32",
    "spec": "Draft17.34",
    "p_head": "773643d2709a5e5db24650069d67c0d9032fefb0"
  },
  "independence": "Initial source and predictions derived solely from Issue1144 and canonical before producer inspection.",
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
      "path": "tests/r1144/sources/add_zero.nl",
      "sha256": "2f99b12fea9bf70d82c27bafe2153ea43af17813e5e1f16e7c5acb0ce877a87e",
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
      "path": "tests/r1144/sources/add_one.nl",
      "sha256": "d7967b1c934d38594d7c7075b5042518b226e6a2463b1af5f35df931c68cceac",
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
      "path": "tests/r1144/sources/add_three.nl",
      "sha256": "71d1681752e3f363a30ede801f6a5463c0192b741f32508f05c2f269e4daf110",
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
      "path": "tests/r1144/sources/sub_zero.nl",
      "sha256": "c941016972d316d096187cf6dfe56e099c6d4c2ca91d026fc0f8d053b5677483",
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
      "path": "tests/r1144/sources/sub_one.nl",
      "sha256": "936e5fe76a882374dc6e2744840086a1eda9151f642c44a4e3f1684bdb133532",
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
      "path": "tests/r1144/sources/sub_three.nl",
      "sha256": "f5aa6f52ac1b93731acdb8ed4882e96daa9550ad78161e4ff0b75885f7dbb21f",
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
      "path": "tests/r1144/negative/underflow_guard.nl",
      "sha256": "70e7be8790dc5fa411989d17e6c86b9d8629603daa7f0502de4cf886f64cbb62",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "quantity precondition / CHECK",
      "reason": "At current=1 the true guard cursor>0 permits 1-2. usize cannot represent -1.",
      "native_allowed": false,
      "unsafe_if_executed": true
    },
    {
      "id": "N2",
      "path": "tests/r1144/negative/carry_type.nl",
      "sha256": "7463dfb0f13968f9cf8f859887781d2c0f2a7d6d44d8c1cdcb1a09900a0d3ec9",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "static carry type / frontend or CHECK",
      "reason": "Continue passes u8 into a usize parameter.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N3",
      "path": "tests/r1144/negative/carry_arity.nl",
      "sha256": "82cc85ac62e715c485725b0e168fab78a43ee43e8e9fed3b1486e6b4235305d2",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "static carry arity / frontend or CHECK",
      "reason": "Two Continue arguments target one parameter.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N4",
      "path": "tests/r1144/negative/result_actual_type.nl",
      "sha256": "7c8fab78db90e6e9b4e728e3b32c0e06ed5d823c6d2a34f59628e75153b85a91",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "ordinary call actual type / frontend or CHECK",
      "reason": "The loop result and its Copy are u8; r1144_pick requires usize. Break result need not equal carry type, so the actual mismatch is the intended invalidity.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N5",
      "path": "tests/r1144/negative/scope_escape_name.nl",
      "sha256": "25160bf8624b77263c51572e83297c3279a16b1b0be6c8a27ca44802cacbc722",
      "classification": "canonical-invalid",
      "expected_check": "reject",
      "predicted_first_rejection_layer": "lexical name resolution / frontend or CHECK",
      "reason": "cursor is scoped to the loop body and cannot be named by the outer Copy binding.",
      "native_allowed": false,
      "unsafe_if_executed": false
    },
    {
      "id": "N6",
      "path": "tests/r1144/negative/even_sub_precision.nl",
      "sha256": "0d892f7154ca367a8ee0747aac1e655ec0b6558ae56b7286f45e56a97b7d2b7e",
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
      "source_selected_result"
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
  }
}
```

**FIRST/RED/NOT_RUN:** この時点で全parser/CHECK/owned/C17/nativeはNOT_RUN。実行後に元予測・SHAを変更しません。有限観測を一般soundness/termination proof/Product PASSへ昇格しません。audit locality UNMEASURED／GO624 PAUSED／H1 HOLD／P1036-PR1050 STOP。
