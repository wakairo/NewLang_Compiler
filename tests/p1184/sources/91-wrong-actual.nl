fn select_phase(phase:usize)->u8 {
 if(phase < usize(2)){u8(53)}else{u8(149)}
}
fn main()->unit {
 let incoming=select_phase(u8(1));
 unit
}
