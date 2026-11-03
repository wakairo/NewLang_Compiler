struct Ledger { old_field: u8, guard_field: u8 }

fn main() -> unit {
    let original = Ledger { old_field: u8(37), guard_field: u8(148) };
    let invalid = loop () {
        break original@absent;
    };
    return unit;
}
