struct Beacons { cold: u8, hot: u8 }

fn return_old(slot: ref<write,u8>, new_value: u8) -> u8 {
    let previous = replace(slot, new_value);
    return previous;
}

fn main() -> unit {
    let west = Beacons { hot: u8(214), cold: u8(39) };
    let east = Beacons { cold: u8(87), hot: u8(166) };
    let prior_hot = loan_write(west@phantom) { |w|
        return_old(w, u8(12))
    };
    let prior_cold = loan_write(east@cold) { |w|
        replace(w, prior_hot)
    };
    let hot_now = west@hot;
    let cold_now = east@cold;
    let cold_stable = west@cold;
    let hot_stable = east@hot;
    unit
}
