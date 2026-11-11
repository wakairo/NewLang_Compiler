struct Sample { datum:u8, stamp:u8 }
fn collect(incoming:u8) -> u8 {
    let sample = Sample { datum:u8(43), stamp:u8(181) };
    let old_result = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let prior = loan_write(sample@datum) { |slot|
                replace(slot,incoming)
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(sample@datum) { |slot|
                replace(slot,u8(233))
            };
            break old;
        }
    };
    let current = sample@datum;
    let sibling = sample@stamp;
    current;
    sibling;
    let copied = old_result;
    copied
}
fn main() -> unit {
    let received = collect(u8(67));
    let copied = received;
    copied;
    unit
}
