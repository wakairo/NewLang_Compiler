struct Pair { left:u8, right:u8, }
fn main()->unit {
    let cedar=Pair{left:u8(37),right:u8(193)};
    let quartz=Pair{left:u8(71),right:u8(241)};
    let used=u8(15); let cap=usize(59);
    let gate=used < cap;
    let chosen=if(gate){let north=u8(104);north}else{let south=u8(206);south};
    let previous_cedar=loan_write(cedar@right){|w|replace(w,chosen)};
    let previous_quartz=loan_write(quartz@left){|w|replace(w,u8(163))};
    previous_cedar;previous_quartz;unit
}
