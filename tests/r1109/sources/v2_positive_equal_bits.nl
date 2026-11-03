struct Marker { first: u8, second: u8 }

fn change_once(target: ref<write,u8>, incoming: u8) -> u8 {
    let prior = replace(target, incoming);
    return prior;
}

fn main() -> unit {
    let untouched = Marker { first: u8(31), second: u8(13) };
    let altered = Marker { second: u8(13), first: u8(31) };
    let snapshot = altered@first;
    let previous = loop () {
        let old = loan_write(altered@first) { |writer|
            change_once(writer, u8(55))
        };
        break old;
    };
    let current = altered@first;
    let sibling = altered@second;
    let other = untouched@first;
    unit
}
