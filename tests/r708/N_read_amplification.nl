fn overwrite(slot:ref<write,u8>, value:u8)->u8 {
    replace(slot,value)
}
fn main()->unit {
    let isolated=u8(44);
    let invalid=loan_read(isolated){|readonly|
        overwrite(readonly,u8(52))
    };
    invalid;unit
}
