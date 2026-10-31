struct Gauge { amber: u8, jade: u8 }
fn main() -> unit {
    let gauge = Gauge{amber: u8(41), jade: u8(173)};
    let escaped = loan_write(gauge@amber) { |slot| slot };
    unit
}
