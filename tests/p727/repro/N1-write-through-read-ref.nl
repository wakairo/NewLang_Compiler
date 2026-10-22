struct SignalFrame {
    beam: u8,
    casing: u8,
}

fn invalid_promote(r: ref<read,u8>, arg: u8) -> unit {
    let prior = replace(r, arg);
    unit
}

fn main() -> unit {
    let device = SignalFrame { beam: u8(67), casing: u8(12) };
    let observed = device@beam;
    unit
}
