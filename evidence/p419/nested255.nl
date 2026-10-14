fn echo(x:u8)->u8{x}
fn relay(y:u8)->u8{echo(y)}
fn main()->unit{
    let a=relay(u8(255));
    let b=relay(a);
    b;
    unit
}
