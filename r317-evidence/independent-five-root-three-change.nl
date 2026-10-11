// Track R independent blind witness for Issue #317; no P fixture reuse.
struct Node { next: Option<ptr<Node>>, prev: Option<ptr<Node>>, child: Option<ptr<Node>>, payload: u8, }
struct DetachedHolder { held_ptr: ptr<Node>, held_allocation: Allocation, held_domain: LifetimeDomain, }
fn erase_link(field: ref<write, Option<ptr<Node>>>, replacement: Option<ptr<Node>>) -> unit {
    replace(field, replacement);
    unit
}
fn main() -> unit {
    match try_allocate_one<Node>() {
      None => {
        unit
      },
      Some(bundle_src) => {
        let OneBacking { allocation, raw } = bundle_src;
        let allocation_src = allocation;
        let empty_slot_src = into_slot<Node>(raw);
        let life_src = lifetime_domain();
        let ptr_src = loan_read(life_src) { |proof_init_src| initialize(empty_slot_src, Node {next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(71) }, proof_init_src) };
        match try_allocate_one<Node>() {
          None => {
            let empty_src = loan_exclusive_read(life_src) { |end_src| destroy(ptr_src, end_src) };
            let raw_src = erase_slot<Node>(empty_src);
            finalize_domain(life_src);
            deallocate(allocation_src, raw_src);
            unit
          },
          Some(bundle_A) => {
            let OneBacking { allocation, raw } = bundle_A;
            let allocation_A = allocation;
            let empty_slot_A = into_slot<Node>(raw);
            let life_A = lifetime_domain();
            let ptr_A = loan_read(life_A) { |proof_init_A| initialize(empty_slot_A, Node {next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(72) }, proof_init_A) };
            match try_allocate_one<Node>() {
              None => {
                let empty_A = loan_exclusive_read(life_A) { |end_A| destroy(ptr_A, end_A) };
                let raw_A = erase_slot<Node>(empty_A);
                finalize_domain(life_A);
                deallocate(allocation_A, raw_A);
                let empty_src = loan_exclusive_read(life_src) { |end_src| destroy(ptr_src, end_src) };
                let raw_src = erase_slot<Node>(empty_src);
                finalize_domain(life_src);
                deallocate(allocation_src, raw_src);
                unit
              },
              Some(bundle_B) => {
                let OneBacking { allocation, raw } = bundle_B;
                let allocation_B = allocation;
                let empty_slot_B = into_slot<Node>(raw);
                let life_B = lifetime_domain();
                let ptr_B = loan_read(life_B) { |proof_init_B| initialize(empty_slot_B, Node {next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(73) }, proof_init_B) };
                match try_allocate_one<Node>() {
                  None => {
                    let empty_B = loan_exclusive_read(life_B) { |end_B| destroy(ptr_B, end_B) };
                    let raw_B = erase_slot<Node>(empty_B);
                    finalize_domain(life_B);
                    deallocate(allocation_B, raw_B);
                    let empty_A = loan_exclusive_read(life_A) { |end_A| destroy(ptr_A, end_A) };
                    let raw_A = erase_slot<Node>(empty_A);
                    finalize_domain(life_A);
                    deallocate(allocation_A, raw_A);
                    let empty_src = loan_exclusive_read(life_src) { |end_src| destroy(ptr_src, end_src) };
                    let raw_src = erase_slot<Node>(empty_src);
                    finalize_domain(life_src);
                    deallocate(allocation_src, raw_src);
                    unit
                  },
                  Some(bundle_C) => {
                    let OneBacking { allocation, raw } = bundle_C;
                    let allocation_C = allocation;
                    let empty_slot_C = into_slot<Node>(raw);
                    let life_C = lifetime_domain();
                    let ptr_C = loan_read(life_C) { |proof_init_C| initialize(empty_slot_C, Node {next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(74) }, proof_init_C) };
                    match try_allocate_one<Node>() {
                      None => {
                        let empty_C = loan_exclusive_read(life_C) { |end_C| destroy(ptr_C, end_C) };
                        let raw_C = erase_slot<Node>(empty_C);
                        finalize_domain(life_C);
                        deallocate(allocation_C, raw_C);
                        let empty_B = loan_exclusive_read(life_B) { |end_B| destroy(ptr_B, end_B) };
                        let raw_B = erase_slot<Node>(empty_B);
                        finalize_domain(life_B);
                        deallocate(allocation_B, raw_B);
                        let empty_A = loan_exclusive_read(life_A) { |end_A| destroy(ptr_A, end_A) };
                        let raw_A = erase_slot<Node>(empty_A);
                        finalize_domain(life_A);
                        deallocate(allocation_A, raw_A);
                        let empty_src = loan_exclusive_read(life_src) { |end_src| destroy(ptr_src, end_src) };
                        let raw_src = erase_slot<Node>(empty_src);
                        finalize_domain(life_src);
                        deallocate(allocation_src, raw_src);
                        unit
                      },
                      Some(bundle_dst) => {
                        let OneBacking { allocation, raw } = bundle_dst;
                        let allocation_dst = allocation;
                        let empty_slot_dst = into_slot<Node>(raw);
                        let life_dst = lifetime_domain();
                        let ptr_dst = loan_read(life_dst) { |proof_init_dst| initialize(empty_slot_dst, Node {next: Option<ptr<Node>>::None, prev: Option<ptr<Node>>::None, child: Option<ptr<Node>>::None, payload: u8(75) }, proof_init_dst) };
                          let prior_0 = loan_read(life_C) { |proof_0| let target_0 = ref_from_ptr(write, ptr_C, proof_0); replace(target_0@prev, Option<ptr<Node>>::Some(ptr_B)) };
                          let prior_1 = loan_read(life_A) { |proof_1| let target_1 = ref_from_ptr(write, ptr_A, proof_1); replace(target_1@next, Option<ptr<Node>>::Some(ptr_B)) };
                          let prior_2 = loan_read(life_src) { |proof_2| let target_2 = ref_from_ptr(write, ptr_src, proof_2); replace(target_2@child, Option<ptr<Node>>::Some(ptr_A)) };
                          let prior_3 = loan_read(life_B) { |proof_3| let target_3 = ref_from_ptr(write, ptr_B, proof_3); replace(target_3@next, Option<ptr<Node>>::Some(ptr_C)) };
                          let prior_4 = loan_read(life_A) { |proof_4| let target_4 = ref_from_ptr(write, ptr_A, proof_4); replace(target_4@prev, Option<ptr<Node>>::Some(ptr_C)) };
                          let prior_5 = loan_read(life_B) { |proof_5| let target_5 = ref_from_ptr(write, ptr_B, proof_5); replace(target_5@prev, Option<ptr<Node>>::Some(ptr_A)) };
                          // Move the actual original B-owned values as one complete ordinary package.
                          let detached = DetachedHolder { held_ptr: ptr_B, held_allocation: allocation_B, held_domain: life_B };
                          let DetachedHolder { held_ptr, held_allocation, held_domain } = detached;
                          let ptr_B_current = held_ptr;
                          let allocation_B_current = held_allocation;
                          let life_B_current = held_domain;
                          let after_prev = loan_read(life_B_current) { |proof_prev| let b_prev = ref_from_ptr(write, ptr_B_current, proof_prev); erase_link(b_prev@prev, Option<ptr<Node>>::None) };
                          let after_next = loan_read(life_B_current) { |proof_next| let b_next = ref_from_ptr(write, ptr_B_current, proof_next); erase_link(b_next@next, Option<ptr<Node>>::None) };
                          let after_a_next = loan_read(life_A) { |proof_a_next| let a_next = ref_from_ptr(write, ptr_A, proof_a_next); replace(a_next@next, Option<ptr<Node>>::Some(ptr_C)) };
                          let empty_dst = loan_exclusive_read(life_dst) { |end_dst| destroy(ptr_dst, end_dst) };
                          let raw_dst = erase_slot<Node>(empty_dst);
                          finalize_domain(life_dst);
                          deallocate(allocation_dst, raw_dst);
                          let empty_C = loan_exclusive_read(life_C) { |end_C| destroy(ptr_C, end_C) };
                          let raw_C = erase_slot<Node>(empty_C);
                          finalize_domain(life_C);
                          deallocate(allocation_C, raw_C);
                          let empty_B_current = loan_exclusive_read(life_B_current) { |end_B_current| destroy(ptr_B_current, end_B_current) };
                          let raw_B_current = erase_slot<Node>(empty_B_current);
                          finalize_domain(life_B_current);
                          deallocate(allocation_B_current, raw_B_current);
                          let empty_A = loan_exclusive_read(life_A) { |end_A| destroy(ptr_A, end_A) };
                          let raw_A = erase_slot<Node>(empty_A);
                          finalize_domain(life_A);
                          deallocate(allocation_A, raw_A);
                          let empty_src = loan_exclusive_read(life_src) { |end_src| destroy(ptr_src, end_src) };
                          let raw_src = erase_slot<Node>(empty_src);
                          finalize_domain(life_src);
                          deallocate(allocation_src, raw_src);
                          unit
                      },
                    }
                  },
                }
              },
            }
          },
        }
      },
    }
}
