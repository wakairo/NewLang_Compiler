struct Sensor { idle: u8, tick: u8 }
fn main() -> unit {
    let sensor = Sensor{tick: u8(158), idle: u8(41)};
    let recovered = loop () {
        let previous = loan_write(sensor@tick) { |write| replace(write, u8(237)) };
        break previous;
    };
    let duplicated = recovered;
    let idle = sensor@idle;
    let tick = sensor@tick;
    return unit;
}
