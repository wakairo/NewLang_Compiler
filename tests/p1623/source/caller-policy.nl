fn sections(stock: usize) -> usize {
    stock / usize(5)
}

fn main() -> unit {
    let stock = usize(46);
    let pick = stock < usize(60);
    let backup = pick;
    let good = if (backup) {
        sections(stock)
    } else {
        let leftover = stock % usize(9);
        leftover
    };
    let try_other = stock < usize(20);
    let alternate = if (try_other) {
        sections(stock)
    } else {
        let leftover = stock % usize(9);
        leftover
    };
    let audit = alternate % usize(4);
    good;
    alternate;
    audit;
    unit
}
