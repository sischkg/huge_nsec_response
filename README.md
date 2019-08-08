# Memory usage issue caused by the implementation of NSEC/NSEC3 Type Bit Maps in PowerDNS Recurosr

## Overview

* PowerDNS Recurosr before 4.2.0, which has problems in NSEC/NSEC3 implementations, use large memory unexpectedly, when crafted records are cached.
* PowerDNS Recursor can limit the number of cached Resource Record entries, but cannot limit the amount of memory usage for them.
* The Attacker causes the victim server to use memory unexpectedly and exhaust it by the crafted Resource Records. Then the victim server performance becomes low or stops service.

## Impact
