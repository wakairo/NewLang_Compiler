struct PulsePair { spare: u8, active: u8 }
fn exchange_sample(destination: ref<write, u8>, incoming: u8) -> u8 {
    return replace(destination, incoming);
}
fn main() -> unit {
    let pair = PulsePair{spare: u8(153), active: u8(78)};
    let recovered = loop () {
        break loan_write(pair@spare) { |channel| exchange_sample(channel, u8(203)) };
    };
    let active_old = loan_write(pair@active) { |channel| replace(channel, recovered) };
    let active_now = pair@active;
    let spare_now = pair@spare;
    return unit;
}
