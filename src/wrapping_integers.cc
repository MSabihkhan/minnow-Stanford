#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  uint32_t raw = n + zero_point.raw_value_;
  return Wrap32 { raw };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint64_t MOD = 1ULL << 32;
  uint64_t offset = raw_value_ - zero_point.raw_value_;
  int64_t diff = checkpoint - offset;
  int64_t k = diff >= 0 ? diff / MOD : -( ( -diff + MOD - 1 ) / MOD );
  uint64_t best_cd = 0;
  uint64_t best_dist = 0;
  bool first = true;

  for ( int64_t dk = k - 1; dk <= k + 1; ++dk ) {
    if ( dk < 0 )
      continue;
    uint64_t candidate = offset + dk * MOD;
    uint64_t dist = ( candidate > checkpoint ) ? ( candidate - checkpoint ) : ( checkpoint - candidate );
    if ( first || dist < best_dist ) {
      best_cd = candidate;
      best_dist = dist;
      first = false;
    }
  }

  return best_cd;
}
