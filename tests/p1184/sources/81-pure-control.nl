fn select_phase(phase:usize)->u8 {
 let selected=if(phase < usize(2)){u8(53)}else{u8(149)};
 let forwarded=selected;
 forwarded
}
fn main()->unit {
 let before=select_phase(usize(0));
 let boundary=select_phase(usize(2));
 let after=select_phase(usize(5));
 return unit;
}
