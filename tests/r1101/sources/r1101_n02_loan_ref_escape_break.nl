struct Ticket { first: u8, second: u8 }

fn main() -> unit {
    let escaped = loop () {
        let temporary = Ticket { first: u8(11), second: u8(25) };
        let bad = loan_write(temporary@first) { |w|
            w
        };
        break bad;
    };
    return unit;
}
