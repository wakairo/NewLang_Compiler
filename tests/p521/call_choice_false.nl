fn relay(x:u8)->u8 { x }
fn main()->unit {
    let a=usize(29); let b=usize(30);
    let selected=if(a==b){u8(108)}else{u8(173)};
    let forwarded=relay(selected);
    selected; forwarded; unit
}
