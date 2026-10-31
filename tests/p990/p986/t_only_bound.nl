struct Gauge { reserve: u8, signal: u8 }

fn trade_signal(cell: ref<write,u8>, incoming: u8) -> u8 {
    let prior = replace(cell, incoming);
    return prior;
}

fn main() -> unit {
    let front = Gauge { signal: u8(107), reserve: u8(31) };
    let back = Gauge { reserve: u8(59), signal: u8(219) };
    let was = loan_write(front@signal) { |w|
        trade_signal(w, u8(73))
    };
    let displaced = loan_write(back@reserve) { |w|
        replace(w, was)
    };
    unit
}
