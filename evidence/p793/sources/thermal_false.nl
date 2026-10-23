struct ThermalGate { desired:u8, stamp:u8 }
fn replace_setting(destination:ref<write,u8>, value:u8)->u8 {
    let former=replace(destination,value);
    return former;
}
fn main()->unit {
    let sensor=ThermalGate{stamp:u8(170),desired:u8(61)};
    let journal=ThermalGate{desired:u8(16),stamp:u8(232)};
    let measured=usize(12);let threshold=usize(5);
    let prior=if (measured < threshold) {
        let abandoned=loan_write(sensor@desired){|port|replace_setting(port,u8(88))};
        abandoned;return unit;
    } else {
        loan_write(sensor@desired){|port|replace_setting(port,u8(105))}
    };
    let actual_current=sensor@desired;
    let previous=loan_write(journal@stamp){|port|replace_setting(port,actual_current)};
    let final=sensor@desired;let untouched=sensor@stamp;
    let saved=journal@stamp;let stable=journal@desired;
    prior;actual_current;previous;final;untouched;saved;stable;unit
}
