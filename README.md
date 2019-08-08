# Memory usage issue caused by the implementation of NSEC/NSEC3 Type Bit Maps in PowerDNS Recurosr

## Overview

* PowerDNS Recurosr before 4.2.0, which has problems in NSEC/NSEC3 implementations, use large memory unexpectedly, when crafted records are cached.
* PowerDNS Recursor can limit the number of cached resource record entries, but cannot limit the amount of memory usage for them.
* The Attacker causes the victim server to use memory unexpectedly and exhaust it by the crafted resource records. Then the victim server performance becomes low or stops service.

## Impact

The Attacker prepares the domain name and its authoritative server and sends many queries to the victim PowerDNS Recursor,
then the amount of the victim server memory usage increases.
Finally, the performance of the service becomes low or down.

## Versions affected

PowerDNS Recursor before 4.2.0

## Versions not affected

PowerDNS Recursor 4.2.0

## Workarounds

Set the cache entries limit([max-cache-entries](https://doc.powerdns.com/recursor/settings.html#setting-max-cache-entries)), which estimated in the condition that 3MB memory is used per one resource record Entry.

## Solutions

Update to PowerDNS Recursor 4.2.0.

## Description

### NSEC/NSEC3

In DNSSEC, NSEC and NSEC3 resource record types are defined to prove that certain domain names or record types do not exist.
The Type Bit Maps field in NSEC/NSEC3 record shows the existence of the types of in its owner.

### Wire Format of Type Bit Maps

The wire format of Type Bit Maps is defined to reduce its size([4.1.2. The Type Bit Maps Field](https://tools.ietf.org/html/rfc4034#section-4.1.2) ).

### Implementation of Type Bit Maps in PowerDNS Recursor before 4.2.0

In PowerDNS Recursor before 4.2.0, Type Bit Maps Field are stored in memory as std::set<uint16_t>([class NSECRecordContent](https://github.com/PowerDNS/pdns/blob/rec-4.1.14/pdns/dnsrecords.hh#L506) ).
One bit in Type Bit Maps Field corresponds to the entry in std::set<uint16_t>.

```c++
class NSECRecordContent : public DNSRecordContent
{
public:
  static void report(void);

// snip

DNSName d_next;
  std::set<uint16_t> d_set;
private:
};
```

In STL(CentOS 7 GCC 4.8.5), std::set is implemented by Red-Black-Tree, and one node includes the Color and pointers to parent, left, right nodes.

```c++
  struct _Rb_tree_node_base
  {
    typedef _Rb_tree_node_base* _Base_ptr;
    typedef const _Rb_tree_node_base* _Const_Base_ptr;

    _Rb_tree_color      _M_color;
    _Base_ptr           _M_parent;
    _Base_ptr           _M_left;
    _Base_ptr           _M_right;
```

If all bits in Type Bit Maps Field set to 1, the size on wire is 8704bytes. However the memory usage on PowerDNS Recursor is about 3MB.
Therefore, the memory usage of PowerDNS Recursor  
increases too large, if it caches the many crafted NSEC/NSEC records set many bits to 1 in the Type Bit Maps field.

```text
Wire Format
size of Type Bit Maps = bit map count x ( Window Block + Bitmap Length + Bitmap ) bytes
                     = 256 x ( 1 + 1 + 32 ) bytes
                     = 8704 bytes

PowerDNS Recursor
size of Type Bit Maps = ( node size of red-black tree ) * 65536 + Overhead bytes
                     = 40 x 65535 + Overhead bytes
                     = 2,621,400 + Overhead bytes
                     ~ 3MB
```

sample code to estimate memory usage: [set-uint16_t-x100.cpp](https://github.com/sischkg/huge_nsec_response/blob/master/set-uint16_t-x100.cpp)

### PowerDNS Recursor Cache Limit

BIND and Unbound can limit the amount of memory for resource records,
However PowerDNS Recursor can limit the number of cached resource record entries, but cannot limit the amount of memory usage for them.
