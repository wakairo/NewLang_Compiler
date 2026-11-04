struct Cinder {
    ash: u8,
    ice: u8,
}

fn renew_ash(target: ref<write,u8>, incoming: u8) -> u8 {
    replace(target, incoming)
}

fn main() -> unit {
    let original = Cinder { ash: u8(43), ice: u8(167) };
    let remote = Cinder { ash: u8(81), ice: u8(214) };
    let old_copy = original@ash;
    let sibling_copy = original@ice;
    let loop_old = loop (phase = usize(2)) {
        if (phase < usize(9)) {
            continue(phase + usize(3));
        } else {
            let prior = loan_write(original@ash) { |slot|
                renew_ash(slot, u8(96))
            };
            break prior;
        }
    };
    let break_current = original@ash;
    let break_sibling = original@ice;
    let break_remote_first = remote@ash;
    let break_remote_second = remote@ice;
    let caller_old = loan_write(original@ash) { |slot|
        renew_ash(slot, u8(201))
    };
    let caller_current = original@ash;
    let caller_sibling = original@ice;
    let caller_remote_first = remote@ash;
    let caller_remote_second = remote@ice;
    let preserved_old = old_copy;
    unit
}
