struct Signal { gate: u8, seal: u8 }
fn change_once(target: ref<write,u8>, incoming: u8) -> u8 {
    replace(target, incoming)
}
fn main() -> unit {
    let original = Signal { gate: u8(19), seal: u8(7) };
    let prior = loan_write(original@phantom) { |writer|
        change_once(writer, u8(61))
    };
    unit
}
