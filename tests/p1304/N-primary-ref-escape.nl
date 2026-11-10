struct Telemetry {
    reading: u8,
    checksum: u8,
}

fn rotate_value(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    let receipt = displaced;
    receipt
}

fn record_primary() -> u8 {
    let frame = Telemetry { reading: u8(83), checksum: u8(211) };
    let escaped = loan_write(frame@reading) { |port|
        port
    };
    let previous = rotate_value(escaped, u8(167));
    let visible = frame@reading;
    let neighbor = frame@checksum;
    previous
}

fn record_secondary(forwarded: u8) -> u8 {
    let frame = Telemetry { checksum: u8(109), reading: u8(37) };
    let previous = loan_write(frame@checksum) { |port|
        rotate_value(port, forwarded)
    };
    let visible = frame@checksum;
    let neighbor = frame@reading;
    previous
}

fn main() -> unit {
    let first_old = record_primary();
    let forwarded = first_old;
    let second_old = record_secondary(forwarded);
    let second_copy = second_old;
    unit
}
