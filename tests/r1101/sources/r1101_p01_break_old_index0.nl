struct Ledger { old_field: u8, guard_field: u8 }

fn main() -> unit {
    let original = Ledger { old_field: u8(37), guard_field: u8(148) };
    let receiver = Ledger { old_field: u8(6), guard_field: u8(209) };
    let old_from_break = loop () {
        let old = loan_write(original@old_field) { |w|
            replace(w, u8(91))
        };
        break old;
    };
    let current_after_break = original@old_field;
    let untouched_sibling = original@guard_field;
    let displaced_receiver = loan_write(receiver@guard_field) { |w|
        replace(w, old_from_break)
    };
    let receiver_after = receiver@guard_field;
    let receiver_sibling = receiver@old_field;
    return unit;
}
