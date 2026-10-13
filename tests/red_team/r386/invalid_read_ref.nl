struct Pair { left: u8, right: u8, }
fn write_scalar(slot: ref<read,u8>, fresh: u8) -> u8 {
    replace(slot, fresh)
}
fn main() -> unit {
    let p = Pair { left: u8(7), right: u8(8), };
    loan_read(p@left) { |r|
        write_scalar(r, u8(10))
    };
    unit
}
