struct Harbor {
    sand: u8,
    tide: u8,
}

fn renew_tide(target: ref<write,u8>, incoming: u8) -> u8 {
    replace(target, incoming)
}

fn main() -> unit {
    let original = Harbor { tide: u8(139), sand: u8(73) };
    let remote = Harbor { sand: u8(159), tide: u8(226) };
    let old_copy = original@tide;
    let sibling_copy = original@sand;
    let loop_old = loop (phase = usize(14)) {
        if (phase < usize(5)) {
            let prior = loan_write(original@tide) { |slot|
                renew_tide(slot, u8(87))
            };
            break prior;
        } else {
            continue(phase - usize(4));
        }
    };
    let break_current = original@tide;
    let break_sibling = original@sand;
    let break_remote_first = remote@sand;
    let break_remote_second = remote@tide;
    let caller_old = loan_write(original@tide) { |slot|
        renew_tide(slot, u8(193))
    };
    let caller_current = original@tide;
    let caller_sibling = original@sand;
    let caller_remote_first = remote@sand;
    let caller_remote_second = remote@tide;
    let preserved_old = old_copy;
    unit
}
