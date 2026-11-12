struct Packet { stable:u8, item:u8 }
fn spin_value(input:u8) -> u8 {
    let inner = Packet { stable:u8(211), item:u8(17) };
    let result = loop(turn=usize(0)) {
        if (turn < usize(3)) {
            let displaced = loan_write(inner@item) { |slot|
                replace(slot,input)
            };
            continue(turn+usize(1));
        } else {
            let old = loan_write(inner@item) { |slot|
                replace(slot,u8(197))
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
fn carry_value(value:u8) -> u8 {
    let original = Packet { stable:value, item:u8(59) };
    let argument = original@stable;
    let returned = spin_value(argument);
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
    let caller = Packet { stable:u8(199), item:u8(29) };
    let before = caller@item;
    let supplied = usize(89);
    let returned = carry_value(supplied);
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
