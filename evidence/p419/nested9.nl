fn echo(x:u8)->u8{x}
fn relay(y:u8)->u8{echo(y)}
fn main()->unit{
    let a=relay(u8(9));
    let b=relay(a);
    b;
    unit
}
