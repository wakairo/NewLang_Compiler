struct Signal { gate: u8, seal: u8 }
fn main() -> unit {
    let original = Signal { gate: u8(19), seal: u8(7) };
    let escaped = loan_write(original@gate) { |writer|
        writer
    };
    unit
}
