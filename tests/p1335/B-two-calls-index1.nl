struct Record { stamp:u8, datum:u8 }
fn gather() -> u8 {
    let record = Record { stamp:u8(157), datum:u8(41) };
    let old_result = loop(phase=usize(0)) {
        if (phase < usize(3)) {
            let prior = loan_write(record@datum) { |slot|
                replace(slot,u8(107))
            };
            continue(phase+usize(1));
        } else {
            let old = loan_write(record@datum) { |slot|
                replace(slot,u8(239))
            };
            break old;
        }
    };
    let current = record@datum;
    let sibling = record@stamp;
    current;
    sibling;
    let copied = old_result;
    copied
}
fn main() -> unit {
    let received = gather();
    let again = gather();
    again;
    let copied = received;
    copied;
    unit
}
