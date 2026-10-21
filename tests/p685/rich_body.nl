struct SensorWindow { floor:u8, limit:u8 }
fn sampling_limit()->u8 { let cap = if (usize(2) < usize(4)) { u8(53) } else { u8(54) }; cap }
fn main()->unit {
    let window = SensorWindow { limit:u8(4), floor:u8(9) };
    let chosen = sampling_limit();
    let before = loan_write(window@limit) { |write_slot| replace(write_slot, chosen) };
    let current = window@limit;
    let retained = window@floor;
    before; chosen; retained; current; unit
}
