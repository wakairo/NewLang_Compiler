fn route_stock(stock: usize, shorten: bool) -> usize {
    let source = stock;
    let mode = shorten;
    let answer = if (mode) {
        let sections = source / usize(5);
        sections;
        sections
    } else {
        let leftover = source % usize(9);
        leftover
    };
    let carried = answer;
    carried;
    carried
}

fn main() -> unit {
    let stock = usize(46);
    let pick = stock < usize(60);
    let backup = pick;
    let good = route_stock(stock, backup);
    let alternate = route_stock(stock, stock < usize(20));
    let audit = alternate % usize(4);
    good;
    alternate;
    audit;
    unit
}
