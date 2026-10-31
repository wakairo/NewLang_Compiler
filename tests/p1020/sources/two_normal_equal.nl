struct DialPad { intensity: u8, spare: u8 }
fn recalibrate(destination: ref<write,u8>, incoming: u8) -> unit {
    if (usize(21) == usize(21)) {
        let displaced = replace(destination, incoming);
        unit
    } else {
        let displaced = replace(destination, incoming);
        unit
    };
    unit
}
fn reserved(destination: ref<write,u8>, incoming: u8) -> unit {
    let displaced = replace(destination, incoming);
    unit
}
fn main() -> unit {
    let pad = DialPad{intensity: u8(37), spare: u8(229)};
    loan_write(pad@intensity) { |target|
        recalibrate(target, u8(83));
    };
    let observed = pad@intensity;
    let sibling = pad@spare;
    unit
}
