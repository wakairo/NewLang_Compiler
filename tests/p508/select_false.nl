fn main()->unit {
    let a=usize(2);
    let b=usize(3);
    let equal=a==b;
    let selected=if (equal) { u8(7) } else { u8(8) };
    let copied=selected;
    copied; unit
}
