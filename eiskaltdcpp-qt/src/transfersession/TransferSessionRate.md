# Transfer session rate

Speed, progress, and time-left for the Qt Transfer View.

## Estimator

Session-mean throughput (constant-rate MLE):

```text
rate      = moved / elapsed
progress  = baseline + moved
remaining = size - progress
eta       = remaining / rate
percent   = progress / size
```

Assumption: future average rate ≈ average so far. Prefer this for “when will it finish?” over a short EMA (EMA is snappier on the wire, noisier for ETA, and resets between segments).

| Term | Meaning |
|------|---------|
| `baseline` (`speedBase` on file/upload) | Bytes already done when the session begins |
| `moved` | Bytes transferred after baseline |
| `progress` | Absolute bytes done now |
| `elapsed` | Wall time since begin (includes stalls and gaps between segments) |
| `size` | Full file, or (download peer) file left at join |

**Effective remaining at begin:** `size - baseline`. After transfers: `size - progress`.

## Scopes

**Upload (peer + file)** — parent keyed by path + peer IP (or the leaf if alone).

- Begin once on first `Starting`; `baseline` = first segment `startPos`
- `moved` = finished segment bytes (`fpos`) + in-flight `segBytes`
- Do not restart the clock between segments; Speed is the session mean (not 0 B/s in gaps)
- `%` / Size use full file size

**Download file group** — parent keyed by path (all peers).

- Begin on first segment tick; `baseline` = queue committed position (`FPOS`)
- `progress` = committed `fpos` + in-flight `segBytes` from each peer

**Download peer** — child under a file group; one session for that peer on this file.

- Session is **not** reset between segments
- `fpos` = bytes from finished segments; `segBytes` = current segment
- On first segment: `leftAtJoin = fileSize - FPOS` (whole-file bytes still left)
- Speed, progress text, and `%` use peer bytes against **leftAtJoin**
- Size column still shows full file size

## Readiness

Publish rate/ETA only when the session has begun, `moved > 0`, and `elapsed ≥ 1 s`. Progress `%` uses `progress` even during that warm-up.

## Display

1. ETA from raw bytes/s; round only the Speed column.
2. Progress bar, `%`, and status text share `progress / size` for that row’s scope.
3. `eta < 0` means unknown.

## Non-goals

- Per-connection EMA in these columns (`UserConnection` may still track it elsewhere)
- Rate-change prediction
- Deduplicating overlapping re-gets (counters are transfer volume; sequential full-file gets are the design case)

## Code

| Unit | Role |
|------|------|
| `TransferSessionRate.h` | Pure rate math |
| `TransferSession` | Scope counters + `writeUi` |
| `TransferSessionRow` | Peer tracking + publish onto a row |
| `TransferGroup` | File-group parent aggregate |
| Transfer View model | Tree / settle / listeners |
