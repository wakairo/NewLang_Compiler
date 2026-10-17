fn echo(x:u8)->u8{x}
fn main()->unit{let low=echo(u8(0));let high=echo(u8(255));low;high;unit}
