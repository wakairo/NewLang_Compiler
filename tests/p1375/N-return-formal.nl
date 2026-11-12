struct Signal { backing:u8, data:u8 }
fn take_displaced(sample:u8) -> u8 {
    let temporary = Signal { backing:sample, data:u8(137) };
    let before = temporary@data;
    let prior = loan_write(temporary@data) { |cell|
        replace(cell,u8(229))
    };
    let current = temporary@data;
    let sibling = temporary@backing;
    before;
    current;
    sibling;
    let copied = sample;
    copied
}
fn main() -> unit {
    let retained = Signal { backing:u8(181), data:u8(43) };
    let result = loop(phase=usize(0)) {
        if (phase < usize(3)) {
            let present = retained@data;
            let carried = take_displaced(present);
            let displaced = loan_write(retained@data) { |slot|
                replace(slot,carried)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(retained@data) { |slot|
                replace(slot,u8(241))
            };
            break old;
        }
    };
    let current = retained@data;
    let sibling = retained@backing;
    result;
    current;
    sibling;
    unit
}
