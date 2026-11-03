struct Beacons { low: u8, high: u8 }

fn previous(slot: ref<write,u8>, next: u8) -> u8 {
    let before = replace(slot, next);
    return before;
}

fn main() -> unit {
    let origin = Beacons { high: u8(173), low: u8(12) };
    let mirror = Beacons { low: u8(57), high: u8(4) };
    let returned = loop () {
        let old = loan_write(origin@high) { |w|
            previous(w, u8(28))
        };
        break old;
    };
    let current = origin@high;
    let untouched_low = origin@low;
    let displaced_mirror = loan_write(mirror@low) { |w|
        replace(w, returned)
    };
    let mirror_current = mirror@low;
    let mirror_untouched_high = mirror@high;
    return unit;
}
