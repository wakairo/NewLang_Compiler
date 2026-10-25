struct RelayPair {
    back: u8,
    front: u8,
}

fn restore_previous(port: ref<write,u8>, incoming: u8) -> u8 {
    let earlier = replace(port, incoming);
    return earlier;
}

fn main() -> unit {
    let console = RelayPair{front: u8(212), back: u8(43)};
    let bystander = RelayPair{back: u8(174), front: u8(26)};
    if (u8(4) == u8(4)) {
        loan_write(console@front) { |open_slot|
            restore_previous(open_slot, u8(65))
        }
    } else {
        loan_write(console@front) { |open_slot|
            restore_previous(open_slot, u8(147))
        }
    };
    let final_front = console@front;
    let preserved_back = console@back;
    let remote_back = bystander@back;
    unit
}
