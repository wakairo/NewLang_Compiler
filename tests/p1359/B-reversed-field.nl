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
    let supplied = u8(91);
    let first = wrap_input(supplied);
    let kept = first;
    let other = u8(157);
    let second = wrap_input(other);
    let second_copy = second;
    let first_after = kept;
    let second_after = second_copy;
    first_after;
    second_after;
    unit
}
