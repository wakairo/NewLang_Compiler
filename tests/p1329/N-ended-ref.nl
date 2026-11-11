struct Sample { datum:u8, stamp:u8 }
fn main() -> unit {
    let sample = Sample { datum:u8(43), stamp:u8(181) };
    let old_result = loop(phase=usize(0)) {
        if (phase < usize(2)) {
            let prior = loan_write(sample@datum) { |slot|
                replace(slot,u8(95))
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
    copied;
    let escaped = loan_write(sample@stamp) { |held| held };
    escaped;
    unit
}
