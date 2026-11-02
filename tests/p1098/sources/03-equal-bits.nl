struct Meter { spare: u8, pulse: u8 }
fn main() -> unit {
    let meter = Meter{pulse: u8(146), spare: u8(83)};
    let recovered = loop () {
        let previous = loan_write(meter@pulse) { |write| replace(write, u8(146)) };
        break previous;
    };
    let displaced = loan_write(meter@spare) { |write| replace(write, recovered) };
    let spare = meter@spare;
    let pulse = meter@pulse;
    return unit;
}
