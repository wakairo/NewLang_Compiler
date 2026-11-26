struct PollDescriptor { reserve_mode: u8, normal_mode: u8 }

fn bounded_bytes(blocks: usize) -> usize {
    if (blocks < usize(1024)) { blocks * usize(64) } else { usize(0) }
}

fn select_mode(bytes: usize) -> u8 {
    if (bytes < usize(4096)) { u8(35) } else { u8(93) }
}

fn install(slot: ref<write,u8>, mode: u8) -> u8 {
    let old = replace(slot, mode);
    old
}

fn install_direct(slot: ref<write,u8>, mode: u8) -> u8 {
    replace(slot, mode)
}

fn main() -> unit {
    let active = PollDescriptor{normal_mode: u8(41), reserve_mode: u8(117)};
    let standby = PollDescriptor{normal_mode: u8(149), reserve_mode: u8(101)};
    let rank = usize(3);
    let blocks = if (rank < usize(8)) { usize(63) } else { usize(64) };
    let planned = bounded_bytes(blocks);
    let selected = select_mode(planned);
    let old_active = loan_write(active@normal_mode) { |slot| install(slot, selected) };
    let active_after = active@normal_mode;
    let active_peer = active@reserve_mode;
    let old_standby = loan_write(standby@reserve_mode) { |slot| install_direct(slot, active_after) };
    let standby_after = standby@reserve_mode;
    let standby_peer = standby@normal_mode;
    let first_previous = old_active;
    let second_previous = old_standby;
    return unit;
}
