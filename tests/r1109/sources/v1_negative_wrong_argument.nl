struct Signal { gate: u8, seal: u8 }
fn change_once(target: ref<write,u8>, incoming: u8) -> u8 {
    replace(target, incoming)
}
fn main() -> unit {
    let original = Signal { gate: u8(19), seal: u8(7) };
    let prior = loop () {
        let old = loan_write(original@gate) { |writer|
            change_once(original@gate, u8(61))
        };
        break old;
    };
    unit
}
