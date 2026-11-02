struct Panel { reserve: u8, active: u8 }

fn main() -> unit {
    let untouched = Panel{reserve: u8(77), active: u8(138)};
    let panel = Panel{reserve: u8(181), active: u8(46)};
    let recovered = loop () {
        let prior = loan_write(panel@active) { |slot| replace(slot, u8(229)) };
        break prior;
    };
    let displaced = loan_write(panel@reserve) { |slot| replace(slot, recovered) };
    let final_active = panel@active;
    let final_reserve = panel@reserve;
    let witness = untouched@active;
    return unit;
}
