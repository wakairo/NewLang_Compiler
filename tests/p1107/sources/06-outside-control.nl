struct PulsePair { spare: u8, active: u8 }
fn exchange_sample(destination: ref<write, u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    let forwarded = displaced;
    return forwarded;
}
fn main() -> unit {
    let pair = PulsePair{spare: u8(64), active: u8(185)};
    let recovered = loan_write(pair@active) { |channel| exchange_sample(channel, u8(226)) };
    let spare_old = loan_write(pair@spare) { |channel| replace(channel, recovered) };
    let active_now = pair@active;
    let spare_now = pair@spare;
    return unit;
}
