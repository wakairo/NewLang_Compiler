struct Gauge {
    sample: u8,
    guard: u8,
}

fn classify(phase: usize) -> u8 {
    if (phase < usize(2)) {
        u8(41)
    } else {
        u8(173)
    }
}

fn main() -> unit {
    let original = Gauge { guard: u8(211), sample: u8(7) };
    let old_snapshot = original@sample;
    let sibling_snapshot = original@guard;
    let loop_result = loop (phase = usize(0)) {
        if (phase < usize(3)) {
            let delivered = classify(phase);
            let replaced = loan_write(original@sample) { |writer|
                replace(writer, delivered)
            };
            let current = original@sample;
            let sibling = original@guard;
            let next_phase = if (phase < usize(1)) {
                usize(1)
            } else {
                if (phase < usize(2)) {
                    usize(2)
                } else {
                    usize(3)
                }
            };
            continue(next_phase);
        } else {
            break original@sample;
        }
    };
    let final_sample = original@sample;
    let final_guard = original@guard;
    let snapshot_after = old_snapshot;
    unit
}
