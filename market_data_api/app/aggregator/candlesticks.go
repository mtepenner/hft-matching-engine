// Package aggregator builds OHLC candlestick data from a stream of trades.
package aggregator

import (
	"sync"
	"time"
)

// Trade represents a matched trade event.
type Trade struct {
	Symbol    string
	Price     float64
	Qty       uint32
	Timestamp time.Time
}

// Candle is an OHLCV candle.
type Candle struct {
	Symbol    string    `json:"symbol"`
	Open      float64   `json:"open"`
	High      float64   `json:"high"`
	Low       float64   `json:"low"`
	Close     float64   `json:"close"`
	Volume    uint64    `json:"volume"`
	OpenTime  time.Time `json:"open_time"`
	CloseTime time.Time `json:"close_time"`
}

// Aggregator accumulates trades into OHLCV bars.
type Aggregator struct {
	mu       sync.Mutex
	interval time.Duration
	current  map[string]*Candle
	closed   []Candle
	OnClose  func(Candle)
}

// New creates an Aggregator with the given bar interval.
func New(interval time.Duration) *Aggregator {
	a := &Aggregator{
		interval: interval,
		current:  make(map[string]*Candle),
	}
	go a.ticker()
	return a
}

// Ingest adds a trade to the current bar.
func (a *Aggregator) Ingest(t Trade) {
	a.mu.Lock()
	defer a.mu.Unlock()

	c, ok := a.current[t.Symbol]
	if !ok {
		now := time.Now().Truncate(a.interval)
		c = &Candle{
			Symbol:   t.Symbol,
			Open:     t.Price,
			High:     t.Price,
			Low:      t.Price,
			Close:    t.Price,
			OpenTime: now,
		}
		a.current[t.Symbol] = c
	}

	if t.Price > c.High {
		c.High = t.Price
	}
	if t.Price < c.Low {
		c.Low = t.Price
	}
	c.Close = t.Price
	c.Volume += uint64(t.Qty)
}

// Recent returns the last N closed candles for a symbol.
func (a *Aggregator) Recent(symbol string, n int) []Candle {
	a.mu.Lock()
	defer a.mu.Unlock()
	var out []Candle
	for i := len(a.closed) - 1; i >= 0 && len(out) < n; i-- {
		if a.closed[i].Symbol == symbol {
			out = append([]Candle{a.closed[i]}, out...)
		}
	}
	return out
}

// ticker closes bars on interval boundaries.
func (a *Aggregator) ticker() {
	for range time.Tick(a.interval) {
		a.mu.Lock()
		now := time.Now()
		for sym, c := range a.current {
			c.CloseTime = now
			a.closed = append(a.closed, *c)
			if a.OnClose != nil {
				a.OnClose(*c)
			}
			delete(a.current, sym)
		}
		a.mu.Unlock()
	}
}
