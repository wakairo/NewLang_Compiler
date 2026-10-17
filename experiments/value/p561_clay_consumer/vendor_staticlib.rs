// V561 Rust vendor-built staticlib C ABI smoke only.
// This is NOT a Clay replacement; confirms packaging/link feasibility on Linux.
#[no_mangle]
pub extern "C" fn p561_rectangle_area(width: u32, height: u32) -> u64 {
    (width as u64) * (height as u64)
}
