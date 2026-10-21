struct Vesper { signal: u8, alarm: u8 }
fn main() -> unit {
    let chime = Vesper { signal: u8(13), alarm: u8(21) };
    let escaped = loan_write(chime@signal) { |borrowed| borrowed };
    escaped;
    unit
}
