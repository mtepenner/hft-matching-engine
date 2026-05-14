// Package sequencer forwards orders from the FIX gateway to the C++ exchange core
// via a shared-memory ring buffer (conceptual: uses a local channel for skeleton).
package sequencer

import (
	"fmt"
	"log"
	"sync/atomic"
	"time"
)

// OrderEvent is sent from the FIX gateway to the matching engine.
type OrderEvent struct {
	ClientID  uint32
	Symbol    [8]byte
	Side      byte // 'B' or 'S'
	Price     int64
	Qty       uint32
	OrderType byte // 'L'=Limit, 'M'=Market
	Timestamp int64
}

// DisruptorClient mimics the LMAX Disruptor pattern for order ingestion.
type DisruptorClient struct {
	sequence   atomic.Int64
	ringBuffer [1 << 14]OrderEvent // 16384-entry pre-allocated ring
	published  atomic.Int64
}

var globalClient = &DisruptorClient{}

// Publish enqueues an OrderEvent. Returns the sequence number.
func Publish(ev OrderEvent) (int64, error) {
	seq := globalClient.sequence.Add(1)
	slot := seq & int64(len(globalClient.ringBuffer)-1)
	ev.Timestamp = time.Now().UnixNano()
	globalClient.ringBuffer[slot] = ev
	globalClient.published.Store(seq)
	return seq, nil
}

// Consume blocks until a new event is available after afterSeq.
func Consume(afterSeq int64, timeout time.Duration) (*OrderEvent, int64, error) {
	deadline := time.Now().Add(timeout)
	for {
		pub := globalClient.published.Load()
		if pub > afterSeq {
			next := afterSeq + 1
			ev := globalClient.ringBuffer[next&int64(len(globalClient.ringBuffer)-1)]
			return &ev, next, nil
		}
		if time.Now().After(deadline) {
			return nil, afterSeq, fmt.Errorf("timeout waiting for event")
		}
		// Spin – in real LMAX Disruptor this uses a WaitStrategy
	}
}

// StartConsumer runs a goroutine that forwards events to the exchange core.
func StartConsumer() {
	go func() {
		seq := int64(-1)
		for {
			ev, next, err := Consume(seq, 5*time.Second)
			if err != nil {
				continue
			}
			seq = next
			log.Printf("[Sequencer] seq=%d sym=%s side=%c price=%d qty=%d",
				seq, ev.Symbol[:], ev.Side, ev.Price, ev.Qty)
			// In production: write to shared-memory segment read by C++ exchange_core
		}
	}()
}
