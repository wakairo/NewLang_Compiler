struct PulsePair { spare: u8, active: u8 }
fn main() -> unit {
    let first = PulsePair{active: u8(111), spare: u8(32)};
    let second = PulsePair{active: u8(84), spare: u8(205)};
    let recovered = loop () {
        let previous = loan_write(second@spare) { |channel| exchange_sample(channel, u8(139)) };
        break previous;
    };
    let active_old = loan_write(second@active) { |channel| replace(channel, recovered) };
    let first_spare = first@spare;
    let first_active = first@active;
    let second_spare = second@spare;
    let second_active = second@active;
    return unit;
}
fn exchange_sample(destination: ref<write, u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    let forwarded = displaced;
    return forwarded;
}
