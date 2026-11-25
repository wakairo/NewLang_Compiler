struct IngressConfig { queue_tag: u8, receive_mode: u8 }

fn mapping_size(pages: usize) -> usize {
    if (pages < usize(1024)) { pages * usize(64) } else { usize(0) }
}

fn configure_mode(destination: ref<write,u8>, bytes: usize) -> u8 {
    let selected = if (bytes < usize(4096)) { u8(3) } else { u8(6) };
    let displaced = replace(destination, selected);
    displaced
}

fn main() -> unit {
    let ingress = IngressConfig{queue_tag: u8(45), receive_mode: u8(11)};
    let lane = usize(5);
    let pages = if (lane < usize(6)) { usize(64) } else { usize(63) };
    let bytes = mapping_size(pages);
    let old = loan_write(ingress@receive_mode) { |writer| configure_mode(writer, bytes) };
    let mode = ingress@receive_mode;
    let tag = ingress@queue_tag;
    let old_copy = old;
    unit;
}
