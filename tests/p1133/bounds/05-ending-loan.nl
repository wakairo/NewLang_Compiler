struct GaugeBox { quota: u8, stamp: u8 }
fn exchange_old(destination: ref<write, u8>, incoming: u8) -> u8 {
    let prior = replace(destination, incoming);
    prior
}
fn main() -> unit {
    let selected = GaugeBox{quota: u8(60), stamp: u8(219)};
    let ambient = GaugeBox{quota: u8(97), stamp: u8(148)};
    let recovered = loop (phase = usize(2)) {
        if (phase < usize(11)) {
            continue(loan_write(selected@quota) { |borrow| borrow });
        } else {
            let prior = loan_write(selected@quota) { |write| exchange_old(write, u8(171)) };
            break prior;
        }
    };
    let displaced = loan_write(selected@stamp) { |write| replace(write, recovered) };
    let quota = selected@quota;
    let stamp = selected@stamp;
    let other_quota = ambient@quota;
    let other_stamp = ambient@stamp;
    return unit;
}
