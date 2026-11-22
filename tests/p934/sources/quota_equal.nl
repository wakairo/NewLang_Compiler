struct PacketQuota { spare:u8, used:u8 }
fn main()->unit {
    let batch=PacketQuota{used:u8(0),spare:u8(255)};
    let delayed=PacketQuota{spare:u8(203),used:u8(41)};
    if(usize(17)==usize(17)){unit}else{unit};
    unit
}
