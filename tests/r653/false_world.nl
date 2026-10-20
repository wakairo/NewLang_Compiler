struct Pair { left:u8, right:u8, }
fn main()->unit {
    let maple=Pair{left:u8(12),right:u8(222)};
    let basalt=Pair{left:u8(94),right:u8(175)};
    let used=usize(87); let cap=usize(29);
    let gate=used < cap;
    let chosen=if(gate){let north=u8(119);north}else{let south=u8(33);south};
    let previous_maple=loan_write(maple@left){|w|replace(w,chosen)};
    let previous_basalt=loan_write(basalt@right){|w|replace(w,u8(148))};
    let maple_left=maple@left; let maple_right=maple@right;
    let basalt_left=basalt@left; let basalt_right=basalt@right;
    previous_maple;previous_basalt;maple_left;maple_right;basalt_left;basalt_right;unit
}
