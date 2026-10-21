struct PacketStamp{marker:u8,channel:u8}
fn main()->unit{ {let expired=PacketStamp{marker:u8(11),channel:u8(22)};unit}; expired@marker;unit}
