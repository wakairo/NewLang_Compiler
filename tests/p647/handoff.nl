struct Pair { left:u8, right:u8, }
fn main()->unit {
    let queued=usize(6); let ingress_capacity=usize(16);
    let ingress=Pair{left:u8(14),right:u8(181)};
    let egress=Pair{left:u8(33),right:u8(207)};
    let may_admit=queued < ingress_capacity;
    let incoming=if(may_admit){let ready=u8(72);ready}else{u8(8)};
    let previous_ingress=loan_write(ingress@left){|slot|replace(slot,incoming)};
    let reserved=usize(16); let egress_capacity=usize(16);
    let may_forward=reserved < egress_capacity;
    let outgoing=if(may_forward){u8(91)}else{previous_ingress};
    let previous_egress=loan_write(egress@left){|slot|replace(slot,outgoing)};
    let ingress_now=ingress@left; let ingress_tag=ingress@right;
    let egress_now=egress@left; let egress_tag=egress@right;
    previous_ingress; previous_egress;
    ingress_now; ingress_tag; egress_now; egress_tag; unit
}
