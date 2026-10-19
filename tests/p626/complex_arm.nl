fn main()->unit {
    let used=usize(11);
    let limit=usize(17);
    let fits=used < limit;
    let then_value=u8(42);
    let else_value=u8(9);
    let choice=if (fits) { let staged=then_value; staged } else { else_value };
    let observed=choice;
    observed;
    unit
}
