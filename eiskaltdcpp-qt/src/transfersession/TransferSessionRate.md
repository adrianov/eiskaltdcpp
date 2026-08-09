# Transfer session rate

Speed, progress, and time-left for the Qt Transfer View.

## Estimator

Session-mean throughput (constant-rate MLE):

```text
rate      = moved / elapsed
progress  = baseline + moved
remaining = fileSize - progress
eta       = remaining / rate
percent   = progress / fileSize
```

Assumption: future average rate ≈ average so far. Prefer this for “when will it finish?” over a short EMA (EMA is snappier on the wire, noisier for ETA, and resets between parts).

| Term | Meaning |
|------|---------|
| `baseline` (`speedBase`) | File bytes already done when the session begins |
| `moved` | Bytes transferred after baseline |
| `progress` | Absolute bytes done now |
| `elapsed` | Wall time since begin (includes stalls and part gaps) |
| `fileSize` | Full object size |

**Effective remaining at begin:** `fileSize - baseline`. After transfers: `fileSize - progress`.

## Scopes

**Upload (peer + file)** — parent keyed by path + peer IP (or the leaf if alone).

- Begin on first `Starting`; `baseline` = first part `startPos`
- `moved` = completed part sizes (`fpos`) + in-flight `getPos()` (`segBytes`)

**Download (our file)** — parent keyed by path (all sources), or the leaf if alone.

- Begin on first part tick; `baseline` = queue committed position (`FPOS`)
- `progress` = committed `fpos` + in-flight source `dpos`

## Readiness

Publish rate/ETA only when the session has begun, `moved > 0`, and `elapsed ≥ 1 s`. Progress `%` uses `progress` even during that warm-up.

## Display

1. ETA from raw bytes/s; round only the Speed column.
2. Progress bar, `%`, and status text share `progress / fileSize`.
3. `eta < 0` means unknown.

## Non-goals

- Per-connection EMA in these columns (`UserConnection` may still track it elsewhere)
- Rate-change prediction
- Deduplicating overlapping re-gets (counters are transfer volume; sequential full-file gets are the design case)

## Code

| Unit | Role |
|------|------|
| `TransferSessionRate.h` | Pure rate math |
| `TransferSession` | Scope counters + UI fields |
| Transfer View model | Tree / settle / listeners |
