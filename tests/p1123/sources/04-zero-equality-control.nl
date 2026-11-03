struct AlloyCell { hot: u8, cool: u8 }
fn retrieve_before(place: ref<write, u8>, installed: u8) -> u8 {
    return replace(place, installed);
}
fn main() -> unit {
    let untouched = AlloyCell{cool: u8(241), hot: u8(39)};
    let selected = AlloyCell{cool: u8(64), hot: u8(177)};
    let recovered = loop () {
        let saved = loan_write(selected@hot) { |write| retrieve_before(write, u8(205)) };
        break saved;
    };
    let displaced = loan_write(selected@cool) { |write| replace(write, recovered) };
    let witness_hot = untouched@hot;
    let witness_cool = untouched@cool;
    let final_hot = selected@hot;
    let final_cool = selected@cool;
    return unit;
}
