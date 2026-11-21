fn main() -> unit {
    let first_input = usize(43);
    let first_flag = usize(2) == usize(2);
    let first = choose_mixed(first_flag, first_input);
    let first_copy = first;
    first_copy;
    let second_input = usize(26);
    let second_flag = usize(1) == usize(2);
    let second = choose_mixed(second_flag, second_input);
    let second_copy = second;
    second_copy;
    first;
    first_input;
    first_flag;
    second_input;
    second_flag;
    unit
}

fn choose_mixed(take_quotient: bool, input: usize) -> usize {
    let flag_copy = take_quotient;
    let copied = input;
    let selected = if (flag_copy) {
        copied / usize(7)
    } else {
        copied % usize(7)
    };
    flag_copy;
    copied;
    let result_copy = selected;
    result_copy
}
