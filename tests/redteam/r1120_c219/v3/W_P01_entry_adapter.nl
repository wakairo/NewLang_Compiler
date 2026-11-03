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

fn main() -> unit {
    let sample_0 = r1120_dispatch_one(usize(0));
    let sample_1 = r1120_dispatch_one(usize(612));
    let sample_2 = r1120_dispatch_one(usize(613));
    let sample_3 = r1120_dispatch_one(usize(657));
    let sample_4 = r1120_dispatch_one(usize(768));
    let sample_5 = r1120_dispatch_one(usize(912));
    let sample_6 = r1120_dispatch_one(usize(913));
    let sample_7 = r1120_dispatch_one(usize(914));
    unit
}
