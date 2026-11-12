struct Packet { item:u8, stable:u8 }
fn roll_input(input:u8) -> u8 {
    let inner = Packet { item:u8(23), stable:u8(173) };
    let result = loop(turn=usize(0)) {
        if (turn < usize(2)) {
            let displaced = loan_write(inner@item) { |slot|
                replace(slot,input)
            };
            continue(turn+usize(1));
        } else {
            let old = loan_write(inner@item) { |slot|
                replace(slot,u8(233))
            };
            break old;
        }
    };
    let current = inner@item;
    let sibling = inner@stable;
    current;
    sibling;
    let copy = result;
    copy
}
fn wrap_input(value:u8) -> u8 {
    let original = Packet { item:u8(61), stable:value };
    let argument = original@stable;
    let returned = roll_input(argument);
    let old = loan_write(original@item) { |slot|
        replace(slot,returned)
    };
    let current = original@item;
    let sibling = original@stable;
    old;
    current;
    sibling;
    let copy = returned;
    copy
}
fn main() -> unit {
    let caller = Packet { item:u8(47), stable:u8(167) };
    let before = caller@item;
    let supplied = u8(111);
    let returned = wrap_input(supplied);
    let old = loan_write(caller@item) { |slot|
        replace(slot,returned)
    };
    let current = caller@item;
    let sibling = caller@stable;
    before;
    old;
    current;
    sibling;
    returned;
    unit
}
