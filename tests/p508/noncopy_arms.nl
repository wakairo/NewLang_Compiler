fn main()->unit {
    let a=usize(2);
    let b=usize(2);
    let equal=a==b;
    let selected=if (equal) { lifetime_domain() } else { lifetime_domain() };
    let copied=selected;
    copied; unit
}
