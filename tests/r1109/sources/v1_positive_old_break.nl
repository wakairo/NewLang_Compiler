struct Signal { gate: u8, seal: u8 }
fn probe() -> u8 {
    let original = Signal { seal: u8(7), gate: u8(19) };
    let bystander = Signal { gate: u8(91), seal: u8(23) };
    let snapshot = original@gate;
    let previous = loop () {
        let old = loan_write(original@gate) { |writer|
            change_once(writer, u8(61))
        };
        break old;
    };
    if (previous == u8(19)) {
        if (snapshot == u8(19)) {
            if (original@gate == u8(61)) {
                if (original@seal == u8(7)) {
                    if (bystander@gate == u8(91)) {
                        u8(1)
                    } else { u8(0) }
                } else { u8(0) }
            } else { u8(0) }
        } else { u8(0) }
    } else { u8(0) }
}
fn change_once(target: ref<write,u8>, incoming: u8) -> u8 {
    let old = replace(target, incoming);
    return old;
}
fn main() -> unit {
    let observed = probe();
    unit
}
