struct Pair{left:u8,right:u8,}
fn main()->unit {
    let a=usize(2);
    let b=usize(2);
    let equal=a==b;
    let p=Pair{left:u8(7),right:u8(9)};let q=Pair{left:u8(8),right:u8(9)};
    let selected=if (equal) { p@left } else { q@left };
    let copied=selected;
    copied; unit
}
