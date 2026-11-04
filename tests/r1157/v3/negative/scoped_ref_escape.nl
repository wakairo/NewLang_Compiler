struct OrbitTile { ember: u8, reserve: u8 }
fn main() -> unit {
    let capsule = OrbitTile { reserve: u8(61), ember: u8(17) };
    let before = capsule@ember;
    let settled = loop (grain = usize(9)) {
        if (grain < usize(9)) {
            continue(grain + usize(1));
        } else {
            break grain;
        }
    };
    let echoed = settled;
    let displaced = if (echoed < usize(10)) {
        loan_write(capsule@ember) { |permit| permit }
    } else {
        loan_write(capsule@ember) { |permit| permit }
    };
    let after = capsule@ember;
    let still = capsule@reserve;
    read(displaced);
    unit
}
