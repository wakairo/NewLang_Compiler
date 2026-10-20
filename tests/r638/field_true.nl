struct Datum { first: u8, second: u8, }
fn main() {
    let slab = Datum { first: u8(31), second: u8(200) };
    let selected = if (usize(1) < usize(2)) {
        let copper = u8(62);
        copper
    } else {
        let nickel = u8(193);
        nickel
    };
    let previous = loan_write(slab@first) { |w| replace(w, selected) };
    let after = slab@first;
    let sibling = slab@second;
}
