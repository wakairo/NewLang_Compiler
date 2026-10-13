struct Pair { left: u8, right: u8, }

fn write_scalar(slot: ref<write,u8>, fresh: u8) -> u8 {
    replace(slot, fresh)
}

fn main() -> unit {
    let alpha = Pair { left: u8(255), right: u8(0), };
    let beta = Pair { left: u8(42), right: u8(255), };
    let next_alpha = u8(1);
    let next_beta = u8(250);
    let again_alpha = u8(30);
    let previous_alpha = loan_write(alpha@left) { |one|
        write_scalar(one, next_alpha)
    };
    let previous_beta = loan_write(beta@left) { |two|
        write_scalar(two, next_beta)
    };
    let prior_second_alpha = loan_write(alpha@left) { |again|
        write_scalar(again, again_alpha)
    };
    previous_alpha;
    previous_beta;
    prior_second_alpha;
    alpha@left;
    alpha@right;
    beta@left;
    beta@right;
    unit
}
