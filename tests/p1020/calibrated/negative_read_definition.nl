struct DialPad { intensity: u8, spare: u8 }
fn a_reject_read(destination: ref<read,u8>, incoming: u8) -> unit {
    let displaced = replace(destination, incoming);
    unit
}
fn main() -> unit {
    unit
}
