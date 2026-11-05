fn bad_arm(phase:usize)->u8 {
 if(phase < usize(2)){u8(53)}else{usize(149)}
}
fn main()->unit {
 let incoming=bad_arm(usize(0));
 unit
}
