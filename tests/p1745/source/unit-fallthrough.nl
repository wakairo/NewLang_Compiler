struct PortConfig { pacing: u8, traffic_class: u8 }

fn reservation_size(slots: usize) -> usize {
    if (slots < usize(512)) { slots * usize(128) } else { usize(0) }
}

fn tune_port(destination: ref<write,u8>, bytes: usize) -> u8 {
    let selected = if (bytes < usize(8192)) { u8(12) } else { u8(27) };
    let displaced = replace(destination, selected);
    displaced
}

fn main() -> unit {
    let port = PortConfig{pacing: u8(31), traffic_class: u8(71)};
    let index = usize(7);
    let slots = if (index < usize(5)) { usize(511) } else { usize(512) };
    let bytes = reservation_size(slots);
    let old = loan_write(port@pacing) { |writer| tune_port(writer, bytes) };
    let pacing = port@pacing;
    let traffic_class = port@traffic_class;
    let old_copy = old;
}
