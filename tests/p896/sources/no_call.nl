struct ChannelProfile { label:u8, route:u8 }
fn main()->unit {
    let active=ChannelProfile{route:u8(73),label:u8(19)};
    let archive=ChannelProfile{label:u8(161),route:u8(10)};
    let selected=if (usize(3) < usize(8)) {u8(103)} else {u8(217)};
    selected;unit
}
