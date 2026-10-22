struct SignalFrame {
    beam: u8,
    casing: u8,
}

fn route_beam(slot: ref<write,u8>, mode: u8) -> unit {
    if (mode == u8(9)) {
        let terminal_old = replace(slot, u8(211));
        return unit;
    } else {
        let normal_old = replace(slot, u8(97));
    };
    unit
}

fn main() -> unit {
    let device = SignalFrame { beam: u8(67), casing: u8(12) };
    let standby = SignalFrame { beam: u8(18), casing: u8(44) };
    let old_copy = device@beam;
    let old_sibling = device@casing;
    loan_write(device@beam) { |actual_ref|
        route_beam(actual_ref, u8(2));
    };
    let post_target = device@beam;
    let post_sibling = device@casing;
    let standby_old = loan_write(standby@casing) { |another_ref|
        replace(another_ref, u8(200))
    };
    let standby_post = standby@casing;
    let standby_sibling = standby@beam;
    unit
}
