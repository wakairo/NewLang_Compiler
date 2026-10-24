struct MeterFlags { stamp:u8, quota:u8 }
fn change(port:ref<write,u8>, replacement:u8)->u8 {
    let former=replace(port,replacement);return former;
}
fn main()->unit {
    let meter=MeterFlags{quota:u8(63),stamp:u8(192)};
    let candidate=usize(13);
    let previous=loan_write(meter@quota){|writer|change(writer,candidate)};
    previous;unit
}
