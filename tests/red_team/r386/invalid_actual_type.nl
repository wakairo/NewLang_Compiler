struct Pair { left: u8, right: u8, }
fn write_scalar(slot: ref<write,u8>, fresh: u8) -> u8 {
    replace(slot, fresh)
}
fn main() -> unit {
    let a = Pair { left: u8(7), right: u8(8), };
    let b = Pair { left: u8(1), right: u8(2), };
    loan_write(a@left) { |w|
        write_scalar(w, b)
    };
    unit
}
