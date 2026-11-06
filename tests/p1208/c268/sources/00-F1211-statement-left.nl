struct Pulse { signal:u8, spare:u8 }
fn choose(step:usize)->u8 {
    if(step < usize(4)){return u8(37);}else{u8(149)};
    return u8(211);
}
fn main()->unit {
    let item=Pulse{signal:u8(19),spare:u8(233)};
    let done=loop(phase=usize(0)){
        if(phase < usize(6)){
            let incoming=choose(phase);
            let old=loan_write(item@signal){|w|replace(w,incoming)};
            continue(phase + usize(1));
        }else{break phase;}
    };
    unit
}
