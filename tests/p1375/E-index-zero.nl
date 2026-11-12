struct Packet { data:u8, saved:u8 }
fn take_old(input:u8) -> u8 {
    let temporary = Packet { saved:input, data:u8(211) };
    let before = temporary@data;
    let old = loan_write(temporary@data) { |slot|
        replace(slot,u8(73))
    };
    let current = temporary@data;
    let sibling = temporary@saved;
    before;
    current;
    sibling;
    let relay = old;
    relay
}
fn main() -> unit {
    let original = Packet { saved:u8(157), data:u8(89) };
    let result = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let actual = original@data;
            let returned = take_old(actual);
            let displaced = loan_write(original@data) { |slot|
                replace(slot,returned)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(original@data) { |slot|
                replace(slot,u8(193))
            };
            break old;
        }
    };
    let current = original@data;
    let sibling = original@saved;
    result;
    current;
    sibling;
    unit
}
