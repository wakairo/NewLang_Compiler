struct PhaseCell { live:u8, peer:u8 }
fn bad_result(phase:usize)->usize {phase}
fn main()->unit {
 let original=PhaseCell{live:u8(26),peer:u8(211)};
 let incoming=bad_result(usize(0));
 let old=loan_write(original@live){|writer|replace(writer,incoming)};
 unit
}
