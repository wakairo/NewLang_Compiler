struct ProbeLink { next:Option<ptr<ProbeLink>>, payload:u8 }
fn detach_probe(edge:ref<write,Option<ptr<ProbeLink>>>, address:ptr<ProbeLink>, allocation:Allocation, domain:LifetimeDomain)->LiveTail {
 let detached=replace(edge,Option<ptr<ProbeLink>>::None);
 return LiveTail{owned_ptr:address,owned_allocation:allocation,owned_domain:domain};
}
fn release_probe(address:ptr<ProbeLink>, allocation:Allocation, domain:LifetimeDomain)->unit {
 loan_read(domain){|stable|let live=ref_from_ptr(read,address,stable);unit};
 let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
 let storage=erase_slot<ProbeLink>(vacant);
 finalize_domain(domain);
 deallocate(allocation,storage);
 unit
}
fn main()->unit {
 
 match try_allocate_one<ProbeLink>(){
  None=>{unit},
  Some(first_backing)=>{
   let OneBacking{allocation,raw}=first_backing;
   let first_allocation=allocation;
   let first_slot=into_slot<ProbeLink>(raw);
   let first_domain=lifetime_domain();
   let first=loan_read(first_domain){|stable|
    initialize(first_slot,ProbeLink{next:Option<ptr<ProbeLink>>::None,payload:u8(29)},stable)
   };
   match try_allocate_one<ProbeLink>(){
    None=>{
     let vacant=loan_exclusive_read(first_domain){|ending|destroy(first,ending)};
     let storage=erase_slot<ProbeLink>(vacant);
     finalize_domain(first_domain);
     deallocate(first_allocation,storage);
     unit
    },
    Some(second_backing)=>{
     let OneBacking{allocation,raw}=second_backing;
     let second_allocation=allocation;
     let second_slot=into_slot<ProbeLink>(raw);
     let second_domain=lifetime_domain();
     let second=loan_read(second_domain){|stable|
      initialize(second_slot,ProbeLink{next:Option<ptr<ProbeLink>>::None,payload:u8(211)},stable)
     };
     let absent=loan_read(first_domain){|stable|
      let link=ref_from_ptr(write,first,stable);
      replace(link@next,Option<ptr<ProbeLink>>::Some(second))
     };
     let seen=loan_read(first_domain){|stable|
      let view=ref_from_ptr(read,first,stable);
      read(view@next)
     };
     match seen{
      None=>{unit},
      Some(address)=>{loan_read(second_domain){|stable|let view=ref_from_ptr(read,address,stable);unit}},
     };
     let LiveTail{owned_ptr,owned_allocation,owned_domain}=loan_read(first_domain){|stable|
      let link=ref_from_ptr(write,first,stable);
      detach_probe(link@next,second,second_allocation,second_domain)
     };
     loan_read(owned_domain){|stable|let view=ref_from_ptr(read,owned_ptr,stable);unit};
     release_probe(owned_ptr,owned_allocation,first_domain);
     let vacant=loan_exclusive_read(first_domain){|ending|destroy(first,ending)};
     let storage=erase_slot<ProbeLink>(vacant);
     finalize_domain(first_domain);
     deallocate(first_allocation,storage);
     unit
    },
   }
  },
 };
 unit
}
