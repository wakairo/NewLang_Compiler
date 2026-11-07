## R1213 / C268 — V1 BLIND SOURCE + ORACLE FROZEN (pre-execution)

**Freeze checkpoint UTC:** 2026-11-06T12:45:18.488Z. **Actual SOURCE AUTHORING START:** 2026-11-06T12:39:45Z (previous [START comment](https://github.com/wakairo/NewLang_Compiler/issues/1213#issuecomment-6685033070)). **Fixed authority:** Compiler/main `bd3aed71d7c912d0096626fd20bb79ddf998af52`; CURRENT_SPEC→Draft17.34. Future P target SHA remains **UNBOUND**. No producer-side tests, PR1210, F examples, or old R1197/R1207 source/oracles/runners read.

**V1 byte rule:** all nine files below are **complete verbatim UTF-8 source bytes**; ASCII subset, LF line endings, exactly one trailing LF. To reconstruct a file, take ONLY the contents of its `newlang` fence including the final LF. Do not normalize/edit or substitute other evidence. The per-file SHA256 shown after each name is computed from these exact V1 bytes before any compiler/AST/native execution. All these cases and numerical oracles are R-independent **predictions, not observations or PASS**.

### Canonical-predicted positive/control cases (not run)

| Case | Source roles / actual finite runtime route predicted | Old Copy; post-current and sibling |
|---|---|---|
| **P01** | `Pulse` only T; `cell` only original root; `live` field index 0; `sentinel` disjoint index 1; `select_for_pass` pure known-direct `usize→u8`; loop parameter `pass=0→1`. Iteration0 early `return u8(37)` then one `continue(1)`; iteration1 normal Copy tail `u8(83)` then one normal `break`. | first `replace` old **149**, second old **37**, `last_old=37`; `live_after=83`, `sentinel_before=sentinel_after=211`. Original root and both field incarnations preserved. |
| **P02** | `Dial` only T; `dial` only original; `marker` index 0 sibling; `offset` index **1** changed, intentionally opposite named-initializer source order; `select_for_turn` known-direct; loop `turn=0→1`. Iteration0 **normal** Copy `46` + continue; iteration1 **early RETURN** `172` + break. | `old0=96`, `old1=46`, `second_old=46`, `offset_after=172`, `marker_before=marker_after=6`. Root/child identity persists; never identify field by initializer position. |
| **P03** | `Relay` only T; `relay` original; `count` index0, `log` index1. `desired` on `iter=0→9`: normal Copy **67** in continue iteration; early RETURN **173** in break iteration. After break, explicitly write the returned old Copy into sibling. | `old0=241`, `old1=displaced=67`; after loop `count=173`, `log_before=18`; subsequent `replace(log,displaced)` returns `old_log=18`; `log_after=67`. No root/child incarnation reminting. |

For all P: **canonical source-eligible expectation**, assuming the shown exact closed forms; source CHECK success is only conditional on actual compiler support. If unsupported, **fail closed before C17, zero certified public-owned output**, with the first failing layer recorded. Each concrete run is predicted to take exactly 2 iterations, 1 Continue and 1 normal Break; this does **not** require the compiler to prove a general termination theorem. Function early-return edges must not join normal IF/loop result. The original T must stay live across scoped field Change and the backedge; Copy of the previous value is not a current field fact. No native result is claimed.

### Canonical-predicted negative cases (not run)

| Case | Only source difference from P01 / required rejection |
|---|---|
| N01 | helper early `return usize(37);` despite declared `->u8`: **semantic return type rejection**. |
| N02 | helper normal ELSE tail `usize(83)` despite declared `->u8`: **normal function result type rejection**; early return edge excluded from join. |
| N03 | helper ELSE `break u8(83);` without any **callee-local** active loop: **source-admissibility rejection**, caller loop target cannot escape named-function boundary. |
| N04 | helper ELSE `continue(usize(1));` without callee-local active loop: **source-admissibility rejection**, no nonlocal Continue. |
| N05 | first scoped `loan_write(cell@live)` body returns its bound `w:ref<write,u8>`: **loan result nonescape / scope-exit compatibility rejection**, cannot launder it into `first_old` or a longer-lived ref. |
| N06 | first `loan_write(cell@unissued)`, where `unissued` is NOT a declared `Pulse` field: **source-owned projection rejection** before making ref/current/owner evidence; no field name/offset fallback. |

A source syntax/profile rejection at an **unrelated earlier layer** is not evidence that the intended semantic negative was enforced. Neither expected-positive syntax nor typed C17/no-UB is proven by this freeze. No source-level way of inventing forged issuer/ValueFact/owner credentials is presumed; those adversarial owned/AST checks await a separately released R executor.

### V1 complete source bytes and per-file SHA256

#### `negative/n01_early_return_wrong_type.nl` — 880 bytes / SHA256 `b11409e584c8f914edaf327014d945e41eafa576880b97366f0fa7e758c7d61c`

```newlang
struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return usize(37);
    } else {
        let ordinary = u8(83);
        let copy_of_ordinary = ordinary;
        copy_of_ordinary
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
```

#### `negative/n02_normal_arm_wrong_type.nl` — 870 bytes / SHA256 `0887e335b6d4be00b15c5f2ac071953b43d6e0a4f8b5cda119209bbdf61e22e8`

```newlang
struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return u8(37);
    } else {
        let ordinary = u8(83);
        let copy_of_ordinary = ordinary;
        usize(83)
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
```

#### `negative/n03_callee_break_escapes.nl` — 802 bytes / SHA256 `a6ba3ee7e9786c5751ffb72786d250e471622a295141a0884ec5544a8f3cd185`

```newlang
struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return u8(37);
    } else {
        break u8(83);
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
```

#### `negative/n04_callee_continue_escapes.nl` — 808 bytes / SHA256 `4cda2965b6eac7e1aa77568a9b0f5a54c394077aeaee163b554495c3c1c7c100`

```newlang
struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return u8(37);
    } else {
        continue(usize(1));
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
```

#### `negative/n05_loan_ref_escape.nl` — 845 bytes / SHA256 `d56dd1f14b2bfb746b40fcf88deac9d822a7a76eb19471fdcdf19c7c9909ac80`

```newlang
struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return u8(37);
    } else {
        let ordinary = u8(83);
        let copy_of_ordinary = ordinary;
        copy_of_ordinary
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@live) { |w|
                w
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
```

#### `negative/n06_unissued_field.nl` — 881 bytes / SHA256 `31f66d37325b48296b0cc51323291e6421d42981d9aa55e31f0428050e2ddf21`

```newlang
struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return u8(37);
    } else {
        let ordinary = u8(83);
        let copy_of_ordinary = ordinary;
        copy_of_ordinary
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@unissued) { |w|
                replace(w, select_for_pass(pass))
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
```

#### `positive/p01_early_then_normal.nl` — 877 bytes / SHA256 `8e685e36d621dbb9d2372348bbb637f318d9e9bb9fc404a5cbd18be32c00e6c5`

```newlang
struct Pulse { live: u8, sentinel: u8 }

fn select_for_pass(pass: usize) -> u8 {
    if (pass == usize(0)) {
        return u8(37);
    } else {
        let ordinary = u8(83);
        let copy_of_ordinary = ordinary;
        copy_of_ordinary
    }
}

fn main() -> unit {
    let cell = Pulse { live: u8(149), sentinel: u8(211) };
    let sentinel_before = cell@sentinel;
    let last_old = loop (pass = usize(0)) {
        if (pass == usize(0)) {
            let first_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            continue(usize(1));
        } else {
            let second_old = loan_write(cell@live) { |w|
                replace(w, select_for_pass(pass))
            };
            break second_old;
        }
    };
    let live_after = cell@live;
    let sentinel_after = cell@sentinel;
    return unit;
}
```

#### `positive/p02_normal_then_early_index1.nl` — 858 bytes / SHA256 `1634afd747cebde9cb11a3391104444da6f494e0a26a0298ea4fdf3829eb00b4`

```newlang
struct Dial { marker: u8, offset: u8 }

fn select_for_turn(turn: usize) -> u8 {
    if (turn == usize(1)) {
        return u8(172);
    } else {
        let ordinary = u8(46);
        let copy_of_ordinary = ordinary;
        copy_of_ordinary
    }
}

fn main() -> unit {
    let dial = Dial { offset: u8(96), marker: u8(6) };
    let marker_before = dial@marker;
    let second_old = loop (turn = usize(0)) {
        if (turn == usize(0)) {
            let old0 = loan_write(dial@offset) { |w|
                replace(w, select_for_turn(turn))
            };
            continue(usize(1));
        } else {
            let old1 = loan_write(dial@offset) { |w|
                replace(w, select_for_turn(turn))
            };
            break old1;
        }
    };
    let offset_after = dial@offset;
    let marker_after = dial@marker;
    return unit;
}
```

#### `positive/p03_old_copy_to_sibling.nl` — 902 bytes / SHA256 `ab4a0bfe68ea61ae9d7adbcbd356dcd2b0996645e6d55dc21c943a4d81164c32`

```newlang
struct Relay { count: u8, log: u8 }

fn desired(iter: usize) -> u8 {
    if (iter == usize(9)) {
        return u8(173);
    } else {
        let ordinary = u8(67);
        let copy_of_ordinary = ordinary;
        copy_of_ordinary
    }
}

fn main() -> unit {
    let relay = Relay { count: u8(241), log: u8(18) };
    let displaced = loop (iter = usize(0)) {
        if (iter == usize(0)) {
            let old0 = loan_write(relay@count) { |w|
                replace(w, desired(iter))
            };
            continue(usize(9));
        } else {
            let old1 = loan_write(relay@count) { |w|
                replace(w, desired(iter))
            };
            break old1;
        }
    };
    let log_before = relay@log;
    let old_log = loan_write(relay@log) { |w|
        replace(w, displaced)
    };
    let count_after = relay@count;
    let log_after = relay@log;
    return unit;
}
```

### Frozen manifest and archive binding

- The accompanying immutable `manifest.json` is UTF-8 / JSON sorted keys, 2-space indent + final LF. It records canonical main, source-authoring START, future executor UNBOUND/mode, all nine file paths, byte sizes, SHA256, per-case roles, oracle details and non-execution conditions. **`manifest.json`: 10,463 bytes, SHA256 `b524a8374b1069273fae494478dc030790c95736b177dfe7a20b7084903a3097`.**
- Deterministic `r1213_c268_blind_v1.zip`: manifest plus nine .nl files; ZIP_STORED without compression, lexicographically sorted entry names (manifest first), timestamp 1980-01-01 00:00:00, `create_system=3`, Unix `0644` entry mode, no directory entries. **Archive size 19,634 bytes; SHA256 `2ad64dd02f648ea354f6169a412dd985978a6dd6e4296e6a8cbd78f1354b822d`.** Complete source and oracles are transcribed above for repository-independent review; ZIP is a local reproduction artifact, not claimed uploaded as a GitHub blob.
- The manifest/ZIP and all original V1 sources and predictions are **immutable after this first freeze**. If any future mechanical correction is needed, preserve V1 and independently freeze new `V2` / new SHA / new pre-run expectations, separately labeled per Process §7.6; never overwrite/redact V1 or regrade FIRST.

### Required later proof obligations and current STOP boundary

On a **separate C execution RELEASE only**, require: (1) full immutable P HEAD pinned before R branch creation, (2) source→semantic CHECK and precise typed RETURN/IF/loop/continue/break semantics with per-edge current state, (3) source-owned immutable T nominal/index/root/incarnation, actual fresh scoped loan + `Change(child)` and sibling preservation, (4) authentic public checked-owned issuer/validator evidence with negative unforgeability and no guessed current/old Copy, (5) strict typed C17 result/body/call completeness and actual native values in independent harness, (6) scoped failure/negative and sanitizer evidence, (7) proper R-FAST branch-wide ledger and the separately authorized natural FULL5+PARTIAL budgets. Each layer may be **NOT TESTED / UNSUPPORTED** without claiming a semantic negative as PASS.

**No actions allowed now**: branch/ref/push/PR = **0**, GitHub Actions/CI/FAST/FULL5/PARTIAL = **0**, compiler/native/C17/AST/Lean/sanitizer = **0**. No P-equivalent local executor now or later. **Future execution method: GitHub-only R-owned branch `r-fast/r1213-helper-return-source` + GitHub Actions**, if separately released. No READY, merge, Product GO; #624 PAUSED, H1 #268 HOLD; UB-prevention audit locality of author+ordinary caller vs well-factored C **UNMEASURED**. STOP now pending separate C release.