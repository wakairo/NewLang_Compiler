struct TideGauge {
    port: u8,
    keel: u8,
}

fn main() -> unit {
    let hull = TideGauge { port: u8(23), keel: u8(34) };
    loan_write(hull@keel) { |loan_local|
        return unit;
    };
    unit
}
