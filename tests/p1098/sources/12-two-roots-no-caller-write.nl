struct Cell { charge: u8, reserve: u8 }
fn main() -> unit {
    let untouched = Cell{charge: u8(117), reserve: u8(206)};
    let selected = Cell{reserve: u8(73), charge: u8(189)};
    let recovered = loop () {
        break loan_write(selected@charge) { |write| replace(write, u8(28)) };
    };
    let duplicated = recovered;
    let first_charge = untouched@charge;
    let first_reserve = untouched@reserve;
    let second_charge = selected@charge;
    let second_reserve = selected@reserve;
    return unit;
}
