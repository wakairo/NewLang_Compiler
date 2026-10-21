struct ValveConfig { high:u8, low:u8 }
fn release_setting()->u8 {
    let valve=ValveConfig{low:u8(174),high:u8(55)};
    let before=loan_write(valve@high){|out|replace(out,u8(90))};
    before
}
fn main()->unit {
    let destination=ValveConfig{low:u8(3),high:u8(4)};
    let supplied=release_setting();
    let previous=loan_write(destination@low){|out|replace(out,supplied)};
    let current=destination@low;
    let sibling=destination@high;
    previous;current;sibling;unit
}
