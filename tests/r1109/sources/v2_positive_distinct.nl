struct Signal { gate: u8, seal: u8 }

fn change_once(target: ref<write,u8>, incoming: u8) -> u8 {
    let prior = replace(target, incoming);
    return prior;
}

fn main() -> unit {
    let original = Signal { seal: u8(7), gate: u8(19) };
    let bystander = Signal { gate: u8(91), seal: u8(23) };
    let snapshot = original@gate;
    let previous = loop () {
        let old = loan_write(original@gate) { |writer|
            change_once(writer, u8(61))
        };
        break old;
    };
    let current = original@gate;
    let sibling = original@seal;
    let other = bystander@gate;
    unit
}
