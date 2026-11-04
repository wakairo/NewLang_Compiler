struct ReedBank { low: u8, high: u8 }
fn collect_prior(destination: ref<write, u8>, replacement: u8) -> u8 {
    let prior = replace(destination, replacement);
    prior
}
fn main() -> unit {
    let focal = ReedBank{low: u8(58), high: u8(193)};
    let witness = ReedBank{low: u8(91), high: u8(217)};
    let recovered = loop (tick = usize(6)) {
        if (tick < usize(7)) {
            continue(u8(7));
        } else {
            let prior = loan_write(focal@high) { |write| collect_prior(write, u8(224)) };
            break prior;
        }
    };
    let displaced = loan_write(focal@low) { |write| replace(write, recovered) };
    let final_low = focal@low;
    let final_high = focal@high;
    let other_low = witness@low;
    let other_high = witness@high;
    return unit;
}
