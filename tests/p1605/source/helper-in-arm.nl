fn groups(bundle: usize) -> usize {
    bundle / usize(7)
}

fn leftover(bundle: usize) -> usize {
    bundle % usize(9)
}

fn dispatch(parcel: usize) -> usize {
    let packed = parcel;
    let selected = if (packed < usize(50)) {
        let near = packed;
        groups(near)
    } else {
        let far = packed;
        leftover(far)
    };
    let copied = selected;
    copied / usize(2)
}

fn main() -> unit {
    let early = dispatch(usize(43));
    let late = dispatch(usize(71));
    unit
}
