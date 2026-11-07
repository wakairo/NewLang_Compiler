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
