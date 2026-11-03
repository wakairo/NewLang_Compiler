fn r1120_dispatch_one(input: usize) -> u8 {
    r1120_policy_one(input)
}

fn r1120_policy_one(input: usize) -> u8 {
    let exactly = input == usize(913);
    if (exactly) {
        u8(47)
    } else {
        let below = input < usize(613);
        if (below) {
            u8(152)
        } else {
            u8(219)
        }
    }
}
