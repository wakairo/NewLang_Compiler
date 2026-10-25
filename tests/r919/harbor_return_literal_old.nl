struct HarborManifest { cargo: u8, seal: u8 }

fn audit_cargo(slot: ref<write,u8>, parcel: u8) -> u8 {
    let retired = replace(slot, parcel);
    let past = retired;
    return u8(216);
}

fn main() -> u8 {
    let vessel = HarborManifest { seal: u8(74), cargo: u8(216) };
    let mooring = HarborManifest { cargo: u8(31), seal: u8(142) };
    if (usize(11) == usize(11)) {
        loan_write(vessel@cargo) { |write_then| audit_cargo(write_then, u8(48)) }
    } else {
        loan_write(vessel@cargo) { |write_else| audit_cargo(write_else, u8(117)) }
    };
    let present = vessel@cargo;
    let untouched = vessel@seal;
    let elsewhere = mooring@cargo;
    return present;
}
