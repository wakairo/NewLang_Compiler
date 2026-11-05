struct FaultCell { hot: u8, cold: u8 }

fn reflect(held: ref<write,u8>) -> ref<write,u8> {
    let copied = held;
    copied
}

fn main() -> unit {
    let root = FaultCell{hot: u8(22), cold: u8(173)};
    let entry_selected = root@hot;
    let entry_sibling = root@cold;
    let starting_phase = usize(0);
    let stopped = loop (phase = usize(0)) {
        if (phase < usize(2)) {
            let escaped = loan_write(root@hot) { |destination|
                reflect(destination)
            };
            let old = replace(escaped, u8(44));
            let current_copy = root@hot;
            let sibling_copy = root@cold;
            continue(phase + usize(1));
        } else {
            break phase;
        }
    };
    let post_selected = root@hot;
    let post_sibling = root@cold;
    let stopped_copy = stopped;
    let starting_phase_copy = starting_phase;
    unit
}
