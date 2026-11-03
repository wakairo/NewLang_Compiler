struct Channel { signal: u8, reserve: u8 }
fn main() -> unit {
    let channel = Channel{reserve: u8(57), signal: u8(194)};
    let recovered = loop () {
        break loan_write(channel@signal) { |write| replace(write, u8(112)) };
    };
    let displaced = loan_write(channel@reserve) { |write| replace(write, recovered) };
    let signal = channel@signal;
    let reserve = channel@reserve;
    return unit;
}
