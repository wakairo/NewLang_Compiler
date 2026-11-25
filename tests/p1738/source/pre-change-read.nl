struct ReceiveSetting { route_tag: u8, mode: u8 }

fn byte_budget(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn negotiate(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(35) } else { u8(93) }
}

fn install(slot: ref<write,u8>, mode: u8) -> u8 {
    let displaced = replace(slot, mode);
    displaced
}

fn main() -> unit {
    let active = ReceiveSetting{mode: u8(41), route_tag: u8(117)};
    let standby = ReceiveSetting{mode: u8(101), route_tag: u8(149)};
    let rank = usize(3);
    let blocks = if (rank < usize(8)) { usize(63) } else { usize(64) };
    let bytes = byte_budget(blocks);
    let selected = negotiate(bytes);
    let fallback = active@mode;
    let first_old = loan_write(active@mode) { |field| install(field, selected) };
    let second_old = loan_write(standby@mode) { |field| install(field, fallback) };
    let active_mode = active@mode;
    let active_tag = active@route_tag;
    let standby_mode = standby@mode;
    let standby_tag = standby@route_tag;
    let displaced_active = first_old;
    let displaced_second = second_old;
    return unit;
}
