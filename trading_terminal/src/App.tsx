import React, { useEffect, useState, useCallback } from 'react';
import { TickerTape } from './components/TickerTape';
import { DepthChart } from './components/DepthChart';
import { OrderEntry } from './components/OrderEntry';

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:8080/ws';

interface L2Snapshot {
  symbol: string;
  ts_ns: number;
  bids: [string, string][];
  asks: [string, string][];
}

const App: React.FC = () => {
  const [snapshot, setSnapshot] = useState<L2Snapshot | null>(null);
  const [ticks, setTicks] = useState([
    { symbol: 'AAPL', price: 150.0, change: 0.0, volume: 5000000 },
    { symbol: 'MSFT', price: 320.0, change: 0.0, volume: 3000000 },
    { symbol: 'TSLA', price: 240.0, change: 0.0, volume: 8000000 },
  ]);

  useEffect(() => {
    const ws = new WebSocket(WS_URL);
    ws.onmessage = (evt) => {
      try {
        const data: L2Snapshot = JSON.parse(evt.data);
        setSnapshot(data);
        setTicks(prev =>
          prev.map(t => {
            if (t.symbol !== data.symbol) return t;
            const newPrice = parseFloat(data.bids[0]?.[0] ?? String(t.price));
            const change = ((newPrice - t.price) / t.price) * 100;
            return { ...t, price: newPrice, change };
          })
        );
      } catch (_) {}
    };
    return () => ws.close();
  }, []);

  const handleOrderSubmit = useCallback(
    (order: { symbol: string; side: 'buy' | 'sell'; price: number; qty: number }) => {
      console.log('[OrderEntry] submit:', order);
      // In production: send to FIX gateway via REST/WebSocket
    },
    []
  );

  const bids = snapshot?.bids.map(([price, qty]) => ({ price, qty })) ?? [];
  const asks = snapshot?.asks.map(([price, qty]) => ({ price, qty })) ?? [];

  return (
    <div style={{ background: '#080814', minHeight: '100vh', color: '#e0e0e0', fontFamily: 'monospace' }}>
      <header style={{ padding: '12px 24px', borderBottom: '1px solid #222', display: 'flex', alignItems: 'center', gap: 16 }}>
        <h1 style={{ margin: 0, fontSize: 20, color: '#00c896' }}>HFT Terminal</h1>
        <span style={{ color: '#888', fontSize: 12 }}>
          {snapshot ? `${snapshot.symbol} · ${new Date(snapshot.ts_ns / 1e6).toLocaleTimeString()}` : 'Connecting…'}
        </span>
      </header>

      <TickerTape ticks={ticks} />

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 320px', gap: 16, padding: 16 }}>
        <DepthChart bids={bids} asks={asks} />
        <OrderEntry onSubmit={handleOrderSubmit} />
      </div>
    </div>
  );
};

export default App;
