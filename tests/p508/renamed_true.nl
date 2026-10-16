fn main()->unit {
    let a=usize(2);
    let b=usize(2);
    let equal=a==b;
    let seven=u8(7);let eight=u8(8);let decision=equal;
    let selected=if (decision) { seven } else { eight };
    let copied=selected;
    copied; unit
}
