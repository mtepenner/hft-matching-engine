package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"

	"github.com/mtepenner/hft-matching-engine/market_data_api/app/aggregator"
	"github.com/mtepenner/hft-matching-engine/market_data_api/app/broadcaster"
)

var agg = aggregator.New(time.Second)

func main() {
	// Start fake L2 simulators for demo symbols
	for _, sym := range []string{"AAPL", "MSFT", "TSLA"} {
		broadcaster.StartSimulator(sym)
	}

	// Wire aggregator events to broadcast
	agg.OnClose = func(c aggregator.Candle) {
		log.Printf("[Candle] %s O=%.2f H=%.2f L=%.2f C=%.2f V=%d",
			c.Symbol, c.Open, c.High, c.Low, c.Close, c.Volume)
	}

	mux := http.NewServeMux()

	// WebSocket endpoint: /ws?symbol=AAPL
	mux.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		broadcaster.Global.ServeWS(w, r)
	})

	// REST: GET /candles?symbol=AAPL&limit=20
	mux.HandleFunc("/candles", func(w http.ResponseWriter, r *http.Request) {
		sym := r.URL.Query().Get("symbol")
		if sym == "" {
			http.Error(w, "symbol required", http.StatusBadRequest)
			return
		}
		candles := agg.Recent(sym, 100)
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(candles)
	})

	// Health
	mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintln(w, `{"status":"ok"}`)
	})

	log.Printf("[MarketDataAPI] Listening on :8080")
	if err := http.ListenAndServe(":8080", mux); err != nil {
		log.Fatalf("server error: %v", err)
	}
}
