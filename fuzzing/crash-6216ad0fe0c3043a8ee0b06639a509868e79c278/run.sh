#!/bin/sh
export ASAN_OPTIONS="allocator_may_return_null=1:max_allocation_size_mb=64:dedup_token_length=50:verbosity=0"
../../done/cryptkvs ./fuzz.ckvs new thekey2 'abcdefghijkCS212 CryptKVSlmnopqrstuvwxyz0123456789-àèç^'
