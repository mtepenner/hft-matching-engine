// Package broadcaster streams L2 order book updates to connected WebSocket clients.
package broadcaster

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	CheckOrigin:     func(r *http.Request) bool { return true },
	ReadBufferSize:  1024,
	WriteBufferSize: 4096,
}

// L2Snapshot is the message sent to clients.
type L2Snapshot struct {
	Symbol    string      `json:"symbol"`
	Timestamp int64       `json:"ts_ns"`
	Bids      [][2]string `json:"bids"` // [price, qty]
	Asks      [][2]string `json:"asks"`
}

// Server manages WebSocket connections and broadcasts order book updates.
type Server struct {
	mu      sync.RWMutex
	clients map[*websocket.Conn]bool
}

var Global = &Server{clients: make(map[*websocket.Conn]bool)}

// ServeWS upgrades the HTTP connection and registers the client.
func (s *Server) ServeWS(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("[WS] upgrade error: %v", err)
		return
	}
	defer conn.Close()

	s.mu.Lock()
	s.clients[conn] = true
	s.mu.Unlock()
	log.Printf("[WS] client connected: %s", conn.RemoteAddr())

	for {
		if _, _, err := conn.ReadMessage(); err != nil {
			break
		}
	}

	s.mu.Lock()
	delete(s.clients, conn)
	s.mu.Unlock()
}

// Broadcast sends a snapshot to all connected clients.
func (s *Server) Broadcast(snap L2Snapshot) {
	data, err := json.Marshal(snap)
	if err != nil {
		return
	}

	s.mu.RLock()
	defer s.mu.RUnlock()

	for conn := range s.clients {
		conn.SetWriteDeadline(time.Now().Add(100 * time.Millisecond))
		if err := conn.WriteMessage(websocket.TextMessage, data); err != nil {
			log.Printf("[WS] write error, dropping client: %v", err)
		}
	}
}

// StartSimulator generates fake L2 updates for demonstration.
func StartSimulator(symbol string) {
	go func() {
		ticker := time.NewTicker(100 * time.Millisecond)
		price := 15000.0
		for range ticker.C {
			price += (float64(time.Now().UnixNano()%7) - 3) * 0.01
			snap := L2Snapshot{
				Symbol:    symbol,
				Timestamp: time.Now().UnixNano(),
				Bids: [][2]string{
					{formatPrice(price - 0.01), "500"},
					{formatPrice(price - 0.02), "1200"},
					{formatPrice(price - 0.05), "3000"},
				},
				Asks: [][2]string{
					{formatPrice(price + 0.01), "400"},
					{formatPrice(price + 0.02), "900"},
					{formatPrice(price + 0.05), "2500"},
				},
			}
			Global.Broadcast(snap)
		}
	}()
}

func formatPrice(p float64) string {
	return fmt.Sprintf("%.2f", p)
}
