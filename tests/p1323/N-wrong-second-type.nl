struct ProbeState { sample:u8, tag:u8 }
fn pulse(phase:usize) -> u8 {
    if (phase < usize(1)) { u8(61) } else { u8(137) }
}
fn main() -> unit {
    let state = ProbeState { sample:u8(17), tag:u8(193) };
    let last = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let next = pulse(phase);
            let old_sample = loan_write(state@sample) { |slot|
                replace(slot,next)
            };
            let old_tag = loan_write(state@tag) { |slot|
                replace(slot,usize(1))
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(state@sample) { |slot|
                replace(slot,u8(229))
            };
            break old;
        }
    };
    let sample_after = state@sample;
    let tag_after = state@tag;
    let receipt = last;
    sample_after;
    tag_after;
    receipt;
    unit
}
