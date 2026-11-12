struct Sample { active:u8, reserve:u8 }
fn exchange_local(input:u8) -> u8 {
    let supplied = input;
    let ephemeral = Sample { active:supplied, reserve:u8(157) };
    let before = ephemeral@active;
    let displaced = loan_write(ephemeral@active) { |slot|
        replace(slot,u8(201))
    };
    let after = ephemeral@active;
    let peer = ephemeral@reserve;
    before;
    after;
    peer;
    let first = displaced;
    let second = first;
    let saved = second;
    saved
}
fn main() -> unit {
    let original = Sample { active:u8(83), reserve:u8(211) };
    let result = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let present = original@active;
            let actual = present;
            let returned = exchange_local(actual);
            let incoming = returned;
            let displaced = loan_write(original@active) { |slot|
                replace(slot,incoming)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(original@active) { |slot|
                replace(slot,u8(197))
            };
            break old;
        }
    };
    let current = original@active;
    let sibling = original@reserve;
    result;
    current;
    sibling;
    unit
}
