struct Gauge { amber: u8, jade: u8 }
fn exchange_alpha(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    return displaced;
}
fn exchange_beta(destination: ref<write,u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    return displaced;
}
fn main() -> unit {
    let gauge = Gauge{amber: u8(41), jade: u8(173)};
    let first_old = loan_write(gauge@amber) { |slot|
        exchange_alpha(slot, u8(67))
    };
    let second_old = loan_write(gauge@amber) { |slot|
        exchange_beta(slot, u8(109))
    };
    unit
}
