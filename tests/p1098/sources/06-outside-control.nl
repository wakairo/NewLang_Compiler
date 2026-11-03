struct Meter { spare: u8, pulse: u8 }
fn main() -> unit {
    let meter = Meter{pulse: u8(146), spare: u8(83)};
    let recovered = loan_write(meter@pulse) { |write| replace(write, u8(229)) };
    let displaced = loan_write(meter@spare) { |write| replace(write, recovered) };
    let spare = meter@spare;
    let pulse = meter@pulse;
    return unit;
}
