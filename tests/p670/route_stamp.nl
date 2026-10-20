struct RouteStamp { channel:u8, marker:u8, }
fn main()->unit {
    let route=RouteStamp{marker:u8(133),channel:u8(54)};
    let old_channel=loan_write(route@channel){|slot|replace(slot,u8(97))};
    let written_channel=route@channel;
    let retained_marker=route@marker;
    old_channel; written_channel; retained_marker; unit
}
