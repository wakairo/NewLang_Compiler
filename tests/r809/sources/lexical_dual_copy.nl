struct RelayCell { next: Option<ptr<RelayCell>>, payload: u8, }
struct SignalDuo { low: u8, high: u8, }
fn update_byte(s:ref<write,u8>,v:u8)->u8{let prior=replace(s,v);return prior;}

fn release_original(r:ref<write,Option<ptr<RelayCell>>>,p:ptr<RelayCell>,a:Allocation,d:LifetimeDomain)->LiveTail{
  let old=replace(r,Option<ptr<RelayCell>>::None);
  let producer_local=LiveTail{owned_ptr:p,owned_allocation:a,owned_domain:d};
  return producer_local;
}
fn retire_original(p:ptr<RelayCell>,a:Allocation,d:LifetimeDomain)->unit{
  loan_read(d){|s|let r=ref_from_ptr(read,p,s);unit};
  let empty=loan_exclusive_read(d){|e|destroy(p,e)};
  let full=erase_slot<RelayCell>(empty);
  finalize_domain(d);
  deallocate(a,full);
  unit
}
fn main()->unit{
  match try_allocate_one<RelayCell>(){
    None=>{unit},
    Some(hpack)=>{
      let OneBacking{allocation,raw}=hpack;
      let ha=allocation;
      let hs=into_slot<RelayCell>(raw);
      let hd=lifetime_domain();
      let hp=loan_read(hd){|s|initialize(hs,RelayCell{next:Option<ptr<RelayCell>>::None,payload:u8(71)},s)};
      match try_allocate_one<RelayCell>(){
        None=>{
          let empty=loan_exclusive_read(hd){|e|destroy(hp,e)};
          let full=erase_slot<RelayCell>(empty);
          finalize_domain(hd);
          deallocate(ha,full);
          unit
        },
        Some(tpack)=>{
          let OneBacking{allocation,raw}=tpack;
          let ta=allocation;
          let ts=into_slot<RelayCell>(raw);
          let td=lifetime_domain();
          let tp=loan_read(td){|s|initialize(ts,RelayCell{next:Option<ptr<RelayCell>>::None,payload:u8(182)},s)};
          let old_none=loan_read(hd){|s|
            let w=ref_from_ptr(write,hp,s);
            replace(w@next,Option<ptr<RelayCell>>::Some(tp))
          };
          let seen=loan_read(hd){|s|let r=ref_from_ptr(read,hp,s);read(r@next)};
          match seen{None=>{unit},Some(q)=>{loan_read(td){|s|let r=ref_from_ptr(read,q,s);unit}}};
          let packet=loan_read(hd){|s|
            let w=ref_from_ptr(write,hp,s);
            release_original(w@next,tp,ta,td)
          };
          // R809 calibrated: both T updates occur after the returned original tail owner.
          let alpha=SignalDuo{low:u8(41),high:u8(172)};
          let beta=SignalDuo{low:u8(94),high:u8(63)};
          let old_copy=loan_write(alpha@high){|w|update_byte(w,u8(206))};
          let evicted=loan_write(beta@low){|w|update_byte(w,old_copy)};
          let updated_alpha=alpha@high;
          let updated_beta=beta@low;
          let sibling_alpha=alpha@low;
          let sibling_beta=beta@high;
          let LiveTail{owned_ptr,owned_allocation,owned_domain}=packet;
          loan_read(owned_domain){|s|let r=ref_from_ptr(read,owned_ptr,s);unit};
          retire_original(owned_ptr,owned_allocation,owned_domain);
          let empty=loan_exclusive_read(hd){|e|destroy(hp,e)};
          let full=erase_slot<RelayCell>(empty);
          finalize_domain(hd);
          deallocate(ha,full);
          unit
        },
      }
    },
  }
}
