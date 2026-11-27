fn main() -> unit {
    let seventeen = usize(17);
    let two = usize(2);
    let value = seventeen + two;
    let narrowed = u8(value);
    let returned = usize(narrowed);
    let same = returned == value;
    unit
}
