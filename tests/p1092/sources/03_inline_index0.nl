struct Thermal { inlet: u8, backup: u8 }

fn main() -> unit {
    let sensor = Thermal{inlet: u8(93), backup: u8(167)};
    let returned = loop () {
        break loan_write(sensor@inlet) { |writer| replace(writer, u8(12)) };
    };
    let old_backup = loan_write(sensor@backup) { |writer| replace(writer, returned) };
    let final_inlet = sensor@inlet;
    let final_backup = sensor@backup;
    return unit;
}
