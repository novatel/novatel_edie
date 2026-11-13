# RANGECMP decompression notes

RANGECMP messages compress measurement data in various ways. EDIE's `RangeDecompressor` converts them into RANGE messages, though some decompressed fields may have lower precision than their uncompressed counterparts. This document describes the decompression algorithms and, where possible, the bounds on their accuracy.

## RANGECMP4/5 lock time

### Algorithm

RANGECMP4/5 messages encode lock time as a 4-bit index into the following table (in milliseconds), defined in [`RangeDecompressor::GetRangeCmp4LockTime`](../src/decoders/oem/src/rangecmp/range_decompressor.cpp):

```math
LT = [0, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144].
```

Index $b$ means the actual lock time is in $[LT[b], LT[b+1])$, or at least $LT[15]$ for $b = 15$.

For each signal and source, let $t$ be the header time of the current observation, $p$ that of the previous observation, $\Delta = t - p$, $b_t$ the current index, and $o_t$ the decompressed lock time. The decompressor outputs:

```math
o_t =
\begin{cases}
LT[b_t], & \Delta > LT[b_t], \\
o_p + \Delta, & \Delta \leq LT[b_t].
\end{cases}
\quad(1)
```

Call an observation where the first case applies an **anchor point**. The first observation of a signal is always an anchor point.

Intuitively: if lock were lost and reacquired since the previous observation, the current lock time would be less than $\Delta$. So when $\Delta \leq LT[b_t]$, lock was certainly continuous, and the output simply advances with header time. When $\Delta > LT[b_t]$, a break is possible, so the decompressor conservatively re-anchors to the smallest lock time consistent with the index. This keeps the output from ever exceeding the actual lock time. And because the table entries double, it also can't fall more than one table entry behind.

### Guarantees

Assume observations arrive in chronological order, header times are observation times, and each index is correct (i.e. $LT[b_t] \leq a_t$ for actual lock time $a_t$, with $b_t$ as large as possible). Then the output satisfies

```math
\ell_t \leq o_t \leq a_t,
\qquad
\ell_t =
\begin{cases}
LT[b_t-1], & b_t > 0, \\
0, & b_t = 0.
\end{cases}
\quad(2)
```

Note that the output may be below $LT[b_t]$. For example, suppose an observation with index 14 is followed 200 ms later by one with index 15. The first output can be $LT[14]=131072$ ms. Since $200 \leq LT[15]$, the decoder does not reset at the second observation, so its output is $131072+200=131272$ ms, even though $LT[15]=262144$ ms. This is still above the guaranteed bound $\ell_t=LT[14]$. (This behaviour is codified in the unit test [`RANGECMP4_OUTPUT_BELOW_LOCK_TABLE_LOWER_BOUND`](../src/decoders/oem/test/range_cmp_test.cpp).)

#### Upper bound

At an anchor point, $o_t=LT[b_t] \leq a_t$. Otherwise, $\Delta \leq LT[b_t] \leq a_t$, so lock cannot have broken since $p$ (a break would make $a_t < \Delta$), and $a_t=a_p+\Delta$. Assuming $o_p \leq a_p$, equation (1) gives $o_t = o_p + \Delta \leq a_t$. By induction from the first observation (an anchor point), the bound always holds.

#### Lower bound

At an anchor point, $o_t=LT[b_t] \geq \ell_t$. Otherwise, let $r$ be the most recent anchor point and $\delta = t - r$. As above, lock has been continuous from $r$ to $t$, so

```math
o_t = LT[b_r] + \delta.
\quad(3)
```

If $b_t = 0$, the bound is trivial. Otherwise, $LT[b_t] \geq 2\cdot LT[b_t - 1]$. If $\delta > LT[b_t - 1]$, then $o_t > LT[b_t - 1]$ by (3). Otherwise, by lock continuity,

```math
a_r = a_t - \delta \geq LT[b_t] - \delta \geq 2\cdot LT[b_t - 1] - \delta \geq LT[b_t - 1],
```

so $b_r \geq b_t-1$ by definition of the RANGECMP4/5 lock time index, and (3) gives $o_t \geq LT[b_t - 1]$.
