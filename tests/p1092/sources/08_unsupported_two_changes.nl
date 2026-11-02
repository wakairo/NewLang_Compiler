struct Panel { reserve: u8, active: u8 }

fn main() -> unit {
    let panel = Panel{reserve: u8(181), active: u8(46)};
    let recovered = loop () {
        let prior = loan_write(panel@active) { |slot| replace(slot, u8(229)) };
        let extra = loan_write(panel@reserve) { |slot| replace(slot, u8(65)) };
        break prior;
    };
    let displaced = loan_write(panel@reserve) { |slot| replace(slot, recovered) };
    let final_active = panel@active;
    let final_reserve = panel@reserve;
    return unit;
}
