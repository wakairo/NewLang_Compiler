// source-eligibility: conditional on main selecting Draft17.35.
// P1272 negative: wrong original Domain must fail closed before any emission.
struct DeliveryTotals { accepted: u8, deferred: u8 }
struct WorkTicket { queued: Option<ptr<WorkTicket>>, quota: u8 }

fn detach_ticket(edge: ref<write,Option<ptr<WorkTicket>>>, ticket: ptr<WorkTicket>,
                 allocation: Allocation, domain: LifetimeDomain) -> LiveTail {
    let detached = replace(edge, Option<ptr<WorkTicket>>::None);
    return LiveTail { owned_ptr: ticket, owned_domain: domain,
                      owned_allocation: allocation };
}

fn retire_ticket(ticket: ptr<WorkTicket>, allocation: Allocation,
                 domain: LifetimeDomain) -> unit {
    loan_read(domain) { |stable_ticket|
        let live_ticket = ref_from_ptr(read, ticket, stable_ticket);
        unit
    };
    let dead_slot = loan_exclusive_read(domain) { |end_ticket|
        destroy(ticket, end_ticket)
    };
    let refund = erase_slot<WorkTicket>(dead_slot);
    finalize_domain(domain);
    deallocate(allocation, refund);
    unit
}

fn main() -> unit {
    match try_allocate_one<WorkTicket>() {
        None => { unit },
        Some(template_packet) => {
            let OneBacking { allocation, raw } = template_packet;
            let template_allocation = allocation;
            let template_slot = into_slot<WorkTicket>(raw);
            let template_domain = lifetime_domain();
            let template_ptr = loan_read(template_domain) { |start_template|
                initialize(template_slot,
                    WorkTicket { queued: Option<ptr<WorkTicket>>::None,
                                 quota: u8(181) }, start_template)
            };
            match try_allocate_one<WorkTicket>() {
                None => {
                    let dead_template = loan_exclusive_read(template_domain) { |end_template|
                        destroy(template_ptr, end_template)
                    };
                    let template_refund = erase_slot<WorkTicket>(dead_template);
                    finalize_domain(template_domain);
                    deallocate(template_allocation, template_refund);
                    unit
                },
                Some(pending_packet) => {
                    let OneBacking { allocation, raw } = pending_packet;
                    let pending_allocation = allocation;
                    let pending_slot = into_slot<WorkTicket>(raw);
                    let pending_domain = lifetime_domain();

                    let sampled_quota = loan_read(pending_domain) { |read_template|
                        let template_ref = ref_from_ptr(read, template_ptr, read_template);
                        read(template_ref@quota)
                    };
                    let retained_quota = sampled_quota;
                    let pending_quota = retained_quota;
                    let pending_ptr = loan_read(pending_domain) { |start_pending|
                        initialize(pending_slot,
                            WorkTicket { queued: Option<ptr<WorkTicket>>::None,
                                         quota: pending_quota }, start_pending)
                    };

                    let previous_queue = loan_read(template_domain) { |attach_guard|
                        let queue_writer = ref_from_ptr(write, template_ptr, attach_guard);
                        replace(queue_writer@queued, Option<ptr<WorkTicket>>::Some(pending_ptr))
                    };
                    let copied_queue = loan_read(template_domain) { |inspect_guard|
                        let queue_reader = ref_from_ptr(read, template_ptr, inspect_guard);
                        read(queue_reader@queued)
                    };
                    let LiveTail { owned_ptr, owned_allocation, owned_domain } = loan_read(template_domain) { |detach_guard|
                        let queue_writer = ref_from_ptr(write, template_ptr, detach_guard);
                        detach_ticket(queue_writer@queued, pending_ptr,
                                      pending_allocation, pending_domain)
                    };
                    retire_ticket(owned_ptr, owned_allocation, owned_domain);
                    let dead_template = loan_exclusive_read(template_domain) { |end_template|
                        destroy(template_ptr, end_template)
                    };
                    let template_refund = erase_slot<WorkTicket>(dead_template);
                    finalize_domain(template_domain);
                    deallocate(template_allocation, template_refund);
                    unit
                }
            }
        }
    };
    unit
}
