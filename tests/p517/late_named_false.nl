struct Pair { left: u8, right: u8, }
fn main() -> unit {
    let operand_a = usize(29);
    let operand_b = usize(30);
    let original = Pair { left: u8(37), right: u8(211), };
    let decision = operand_a == operand_b;
    let selected = if (decision) { u8(108) } else { u8(173) };
    let former = loan_write(original@left) { |write_field| replace(write_field, selected) };
    let now_left = original@left;
    let sibling = original@right;
    former;
    now_left;
    sibling;
    unit
}
