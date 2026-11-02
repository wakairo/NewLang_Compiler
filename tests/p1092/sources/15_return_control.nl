struct Panel { reserve: u8, active: u8 }

fn main() -> unit {
    let panel = Panel{reserve: u8(181), active: u8(46)};
    let no_normal_result = loop () {
        let prior = loan_write(panel@active) { |slot| replace(slot, u8(229)) };
        return unit;
    };
    return unit;
}
