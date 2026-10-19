fn main()->unit {
    let used=usize(13);
    let limit=usize(19);
    let fits=used < limit;
    let then_value=u8(42);
    let else_value=u8(9);
    let fits_copy=fits;
    let choice=if (fits_copy) { then_value } else { else_value };
    let observed=choice;
    observed;
    unit
}
