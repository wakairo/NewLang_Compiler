fn dispatch(parcel: usize) -> usize {
    let packed = parcel;
    let selected = if (packed < usize(50)) {
        let near = packed;
        near / usize(7)
    } else {
        let far = packed;
        far % usize(9)
    };
    let copied = selected;
    copied / usize(2)
}

fn main() -> unit {
    let early = dispatch(usize(43));
    let late = dispatch(usize(71));
    unit
}
