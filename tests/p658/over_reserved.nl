struct Dispatch { ready:u8, channel:u8, }
fn main()->unit {
    let payload_bytes=usize(27);
    let header_bytes=usize(5);
    let budget_bytes=usize(96);
    let reserved_bytes=usize(97);
    let framed_bytes=payload_bytes + header_bytes;
    let headroom_bytes=budget_bytes - reserved_bytes;
    let dispatch=Dispatch{ready:u8(2),channel:u8(9)};
    let fits_with_reserve=framed_bytes < headroom_bytes;
    let next_ready=if(fits_with_reserve){u8(1)}else{u8(0)};
    let previous_ready=loan_write(dispatch@ready){|ready|replace(ready,next_ready)};
    let observed_ready=dispatch@ready;
    let observed_channel=dispatch@channel;
    previous_ready; observed_ready; observed_channel; unit
}
