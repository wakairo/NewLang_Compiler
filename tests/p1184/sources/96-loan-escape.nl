struct PhaseCell { live:u8, peer:u8 }
fn escaping(phase:usize)->ref<write,u8> {
 let local=PhaseCell{live:u8(7),peer:u8(9)};
 loan_write(local@live){|writer|writer}
}
fn main()->unit {let result=escaping(usize(0));unit}
