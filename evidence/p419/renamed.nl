fn leaf(input:u8)->u8{input}
fn forward(argument:u8)->u8{leaf(argument)}
fn main()->unit{
    let a=forward(u8(9));
    let b=forward(a);
    b;
    unit
}
