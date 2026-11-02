struct Spare { spare_a: u8, spare_b: u8 }
struct Panel { reserve: u8, active: u8 }

fn main() -> unit {
    let untouched = Spare{spare_a: u8(77), spare_b: u8(138)};
    let panel = Panel{reserve: u8(181), active: u8(46)};
    let recovered = loop () {
        let prior = loan_write(panel@active) { |slot| replace(slot, u8(229)) };
        break prior;
    };
    let displaced = loan_write(panel@reserve) { |slot| replace(slot, recovered) };
    let final_active = panel@active;
    let final_reserve = panel@reserve;
    let witness = untouched@spare_b;
    return unit;
}
