fn echo(value:u8)->u8 { value }
fn main()->unit {
    let a=usize(3);
    let b=usize(9);
    let smaller=a < b;
    let selected=if(smaller){echo(u8(37))}else{echo(u8(82))};
    selected;
    unit
}
