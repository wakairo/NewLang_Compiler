fn main()->unit {
    let demand=usize(57); let limit=usize(45);
    let permitted=demand < limit;
    let chosen=if(permitted){let staged=u8(83);staged}else{u8(24)};
    chosen; unit
}
