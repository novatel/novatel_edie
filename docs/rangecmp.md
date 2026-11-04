# RANGECMP decompression notes

RANGECMP messages compress measurement data in various ways. EDIE's `RangeDecompressor` converts them into RANGE messages, though some decompressed fields may have lower precision than their uncompressed counterparts. This document describes the decompression algorithms and, where possible, the bounds on their accuracy.

## RANGECMP3/4/5 lock time

Lock time is computed by [`RangeDecompressor::GetRangeCmp4LockTime`](../src/decoders/oem/src/rangecmp/range_decompressor.cpp). Defined within this function is the 16-entry lock time table, repeated below:

```math
LT = [0, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144].
```

Each index of this table represents the bucket from its entry up to the next; index 15 has no upper limit.

For the arguments below, assume observations for the same signal and source arrive in chronological order, their header timestamps represent the observation times, and the encoded indices match the actual lock times (i.e. if the actual lock time of a signal is $a$, then the RANGECMPX log contains the largest index $b$ such that $LT[b] \leq a$). All times and table entries are in milliseconds. Let $t$ be the time of the current observation, $p$ the time of the immediately previous observation, and $\Delta = t - p$. Let $b_t$ be the current lock-time index, $a_t$ the actual lock time, and $o_t$ the decompressed lock time.

At a signal's first observation, the algorithm "anchors" its lock time to the given lock table index. For subsequent observations, the decompressor accumulates lock time using header timestamps. This continues until it sees a lock time smaller than the gap between the previous two observations, at which point it "anchors" the signal again. More formally, the output of the algorithm for the message at time $t$ is defined as follows:

```math
o_t =
\begin{cases}
LT[b_t], & \Delta > LT[b_t], \\
o_p + \Delta, & \Delta \leq LT[b_t].
\end{cases}
\quad(1)
```

Call an observation where the first case applies an **anchor point**. Between anchor points, the output advances at the same rate as time and cannot "jump" forwards or backwards.

One might hope/expect that the output of the algorithm is always at least $LT[b_t]$; unfortunately, this is _not_ the case. The bound we can guarantee, relative to the current index, is the _previous_ table entry (or zero for index zero), as follows:

```math
\ell_t =
\begin{cases}
LT[b_t-1], & b_t > 0, \\
0, & b_t = 0,
\end{cases}
\qquad
\ell_t \leq o_t \leq a_t.
\quad(2)
```

For example, suppose an observation with index 14 is followed 200 ms later by one with index 15. The first output can be $LT[14]=131072$ ms. Since $200 \leq LT[15]$, the decoder does not reset at the second observation, so its output is $131072+200=131272$ ms, even though $LT[15]=262144$ ms. This is still above the guaranteed bound $\ell_t=LT[14]$. (This behaviour is codified in the unit test [`RANGECMP4_OUTPUT_BELOW_LOCK_TABLE_LOWER_BOUND`](../src/decoders/oem/test/range_cmp_test.cpp).)

### Why the output does not exceed actual lock time

At an anchor point, $o_t=LT[b_t]$, which is no greater than $a_t$ by the table definition. Otherwise, $\Delta \leq LT[b_t] \leq a_t$. If lock had been lost and reacquired after $p$, the current actual lock time would be less than $\Delta$. So lock was continuous since $p$, and $a_t=a_p+\Delta$.

Assuming the previous output was no greater than actual lock time at $p$, equation (1) gives $o_t = o_p + \Delta \leq a_p + \Delta = a_t$. Starting from the first anchor point, the upper bound therefore holds at every observation.

### Why the output is at least the previous table entry

At an anchor point, $o_t=LT[b_t]$, so the lower bound in (2) holds directly. Now consider an observation that is not an anchor point, and let $r$ be the most recent preceding anchor point. Write $\delta = t - r$. As in the upper-bound argument, every (non-anchor) observation since $r$ confirms the lock hasn't broken since its predecessor. Lock has therefore been continuous from $r$ to $t$, and the output has accumulated elapsed time:

```math
o_t = LT[b_r] + \delta.
\quad(3)
```

For indices 0 and 1, $\ell_t=0$, so the bound follows immediately. Now consider $b_t \geq 2$. Beyond index 0, consecutive values of the lock table double, meaning $LT[b_t] \geq 2\cdot LT[b_t - 1]$.

If $\delta > LT[b_t - 1]$, equation (3) gives $o_t \geq \delta > LT[b_t - 1]$. Otherwise, $\delta \leq LT[b_t - 1]$. Lock continuity and the current table lower bound then give

```math
a_r = a_t - \delta \geq LT[b_t] - \delta \geq 2\cdot LT[b_t - 1] - \delta \geq LT[b_t - 1].
```

Therefore the index at $r$ is at least $b_t-1$, so $LT[b_r] \geq LT[b_t - 1]$. Equation (3) now gives $o_t \geq LT[b_t - 1] + \delta \geq LT[b_t - 1]$. In either case, $o_t \geq LT[b_t-1] = \ell_t$, completing the lower-bound argument.
