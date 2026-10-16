fn main()->unit {
    let a=usize(2);
    let b=usize(2);
    let equal=a==b;
    let x=u8(0);
    let selected=if (equal) { loan_read(x){|r|r} } else { loan_read(x){|r|r} };
    let copied=selected;
    copied; unit
}
