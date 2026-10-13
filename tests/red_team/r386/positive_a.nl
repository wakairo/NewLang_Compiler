struct Pair { left: u8, right: u8, }

fn write_scalar(slot: ref<write,u8>, fresh: u8) -> u8 {
    replace(slot, fresh)
}

fn main() -> unit {
    let alpha = Pair { left: u8(0), right: u8(199), };
    let beta = Pair { left: u8(255), right: u8(13), };
    let next_alpha = u8(240);
    let next_beta = u8(1);
    let previous_alpha = loan_write(alpha@left) { |a_write|
        write_scalar(a_write, next_alpha)
    };
    let previous_beta = loan_write(beta@left) { |b_write|
        write_scalar(b_write, next_beta)
    };
    previous_alpha;
    previous_beta;
    alpha@left;
    alpha@right;
    beta@left;
    beta@right;
    unit
}
