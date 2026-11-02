struct Panel { reserve: u8, active: u8 }

fn main() -> unit {
    let panel = Panel{reserve: u8(181), active: u8(46)};
    let next = u8(229);
    let recovered = loop () {
        let prior = loan_write(panel@active) { |slot| replace(slot, next) };
        break prior;
    };
    let displaced = loan_write(panel@reserve) { |slot| replace(slot, recovered) };
    let final_active = panel@active;
    let final_reserve = panel@reserve;
    return unit;
}
