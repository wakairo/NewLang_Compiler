struct PacketStamp { marker:u8, channel:u8, }
fn main()->unit {
    let stamp=PacketStamp{channel:usize(43),marker:u8(115)};
    let old_marker=loan_write(stamp@marker){|slot|replace(slot,u8(126))};
    let written_marker=stamp@marker;
    let retained_channel=stamp@channel;
    old_marker; written_marker; retained_channel; unit
}
