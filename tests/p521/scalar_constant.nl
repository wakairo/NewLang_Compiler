fn relay(x:u8)->u8 { x }
fn main()->unit { let forwarded=relay(u8(108)); forwarded; unit }
