struct Marker { first: u8, second: u8 }
fn change_once(target: ref<write,u8>, incoming: u8) -> u8 {
    replace(target, incoming)
}
fn probe() -> u8 {
    let untouched = Marker { first: u8(31), second: u8(13) };
    let altered = Marker { second: u8(13), first: u8(31) };
    let previous = loop () {
        let old = loan_write(altered@first) { |writer|
            change_once(writer, u8(55))
        };
        break old;
    };
    if (previous == u8(31)) {
        if (untouched@first == u8(31)) {
            if (altered@first == u8(55)) {
                if (untouched@second == u8(13)) {
                    if (altered@second == u8(13)) {
                        u8(1)
                    } else { u8(0) }
                } else { u8(0) }
            } else { u8(0) }
        } else { u8(0) }
    } else { u8(0) }
}
fn main() -> unit {
    let observed = probe();
    unit
}
